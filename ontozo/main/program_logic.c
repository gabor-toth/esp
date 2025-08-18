#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "gpio_define.h"
#include "gpio_logic.h"
#include "program_logic.h"
#include "program.h"

#define COMMAND_START           1
#define COMMAND_STOP            2
#define COMMAND_NEXT_ZONE       3
#define COMMAND_NEXT_PROGRAM    4
#define COMMAND_QUEUE_START     5
#define COMMAND_PUMP_STATE      6

static const char *LOG_TAG = "program_logic";

static const char *command_names[] = {
        "unknown",
        "start",
        "stop",
        "next_zone",
        "next_program",
        "queue",
        "pump_state",
};

typedef struct {
    int command;
    int program;
    bool pump_state;
} program_command_t;

struct queue_item_t {
    int program_index;
    struct queue_item_t *next;
};

static bool is_running = false;
static int current_program_index;
static int current_zone_index;
static Program *current_program;
static TimerHandle_t timer;
static struct queue_item_t *queue = NULL;

static QueueHandle_t event_queue = NULL;

static void stop_and_move_to_next_program();

static void end_current_zone() {
    if ( current_zone_index >= 0 && current_program != NULL && current_zone_index < current_program->zones_count ) {
        gpio_set_pin_state( OUTPUTS, ZONES_CLASS, current_program->zones[ current_zone_index ].zone_id, false );
    }
}

static void start_next_zone() {
    if ( !is_running ) {
        ESP_LOGW( LOG_TAG, "No program is running" );
        return;
    }
    end_current_zone();
    if ( ++current_zone_index == current_program->zones_count ) {
        ESP_LOGI( LOG_TAG, "Ending program %d after last zone", current_program_index );
        current_zone_index = -1;
        stop_and_move_to_next_program();
        return;
    }
    ProgramZone *zone = &current_program->zones[ current_zone_index ];
    ESP_LOGI( LOG_TAG, "moving to zone %d/%d: id %d, duration %d secs",
              current_zone_index + 1, current_program->zones_count, zone->zone_id, zone->duration_in_seconds );
    gpio_set_pin_state( OUTPUTS, ZONES_CLASS, zone->zone_id, true );
    TickType_t timer_ticks = pdMS_TO_TICKS( zone->duration_in_seconds * 1000 );
    ESP_LOGI( LOG_TAG, "set timer to %ld ticks", timer_ticks );
    if ( !xTimerChangePeriod( timer, timer_ticks, portMAX_DELAY ) ) {
        ESP_LOGE( LOG_TAG, "xTimerChangePeriod failed, aborting program" );
        program_logic_stop();
        return;
    }
    if ( !xTimerReset( timer, portMAX_DELAY ) ) {
        ESP_LOGE( LOG_TAG, "xTimerReset failed, aborting program" );
        program_logic_stop();
        return;
    }
}

static void fire_command( int command, int program, bool pump_state ) {
    program_command_t program_command = {
            .command = command,
            .program = program,
            .pump_state = pump_state
    };
    xQueueSend( event_queue, &program_command, 0 );
}

static void timer_callback( TimerHandle_t unused ) {
    fire_command( COMMAND_NEXT_ZONE, current_program_index, false );
}

static void start_program( int index ) {
    current_program = program_get( index );
    if ( current_program == NULL ) {
        ESP_LOGW( LOG_TAG, "Program does %d not exists, skipping", index );
        stop_and_move_to_next_program();
        return;
    }
    if ( !current_program->valid ) {
        ESP_LOGW( LOG_TAG, "Program %d is not valid, skipping", index );
        stop_and_move_to_next_program();
        return;
    }
    ESP_LOGI( LOG_TAG, "Starting program %d", index );
    is_running = true;
    current_program_index = index;
    current_zone_index = -1;
    gpio_pump_main( true );
    start_next_zone();
}

static void start_or_queue_program( int index ) {
    if ( !is_running ) {
        start_program( index );
        return;
    }
    struct queue_item_t *item = calloc( 1, sizeof( struct queue_item_t ) );
    item->program_index = index;
    if ( queue == NULL ) {
        queue = item;
    } else {
        struct queue_item_t *next = queue;
        while ( next->next != NULL ) {
            next = next->next;
        }
        next->next = item;
    }
    ESP_LOGI( LOG_TAG, "Queued program %d", index );
}

static void stop_current_program() {
    if ( is_running ) {
        ESP_LOGI( LOG_TAG, "Stopping program %d", current_program_index );
    } else {
        ESP_LOGW( LOG_TAG, "No current program to stop" );
    }

    xTimerStop( timer, portMAX_DELAY );
    end_current_zone();
}

static void stop_and_move_to_next_program() {
    stop_current_program();

    if ( queue == NULL ) {
        gpio_pump_main( false );
        is_running = false;
        current_zone_index = -1;
        ESP_LOGI( LOG_TAG, "No queued program to start" );
        return;
    }
    int index = queue->program_index;
    struct queue_item_t *next = queue->next;
    free( queue );
    queue = next;
    start_program( index );
}

static void stop_all_programs() {
    while ( queue != NULL ) {
        struct queue_item_t *next = queue->next;
        free( queue );
        queue = next;
    }
    stop_and_move_to_next_program();
}

static void pump_state_changed( bool is_on ) {
    // TODO add logic
}

_Noreturn static void task_main( void *unused ) {
    for ( ;; ) {
        program_command_t data;
        if ( xQueueReceive( event_queue, &data, portMAX_DELAY ) ) {
            int command = data.command;
            int program = data.program;
            ESP_LOGI( LOG_TAG, "Command %s for program %d received ", command_names[ command ], program );
            if ( ( command == COMMAND_NEXT_ZONE || command == COMMAND_NEXT_PROGRAM || command == COMMAND_STOP ) &&
                 current_program_index != program ) {
                ESP_LOGW( LOG_TAG, "Current program %d != sent program %d, ignoring command %d",
                          current_program_index, program, command );
                continue;
            }
            switch ( command ) {
                case COMMAND_START:
                    start_program( program );
                    break;
                case COMMAND_QUEUE_START:
                    start_or_queue_program( program );
                    break;
                case COMMAND_NEXT_ZONE:
                    start_next_zone();
                    break;
                case COMMAND_NEXT_PROGRAM:
                    stop_and_move_to_next_program();
                    break;
                case COMMAND_STOP:
                    stop_all_programs();
                    break;
                case COMMAND_PUMP_STATE:
                    pump_state_changed( data.pump_state );
                    break;
                default:
                    ESP_LOGE( LOG_TAG, "Unhandled command %d", command );
                    break;
            }
        }
    }
}

void program_logic_start( int index ) {
    fire_command( COMMAND_QUEUE_START, index, false );
}

void program_logic_move_to_next_zone() {
    fire_command( COMMAND_NEXT_ZONE, current_program_index, false );
}

void program_logic_move_to_next_program() {
    fire_command( COMMAND_NEXT_PROGRAM, current_program_index, false );
}

void program_logic_stop() {
    fire_command( COMMAND_STOP, current_program_index, false );
}

void program_logic_get_state( RunningProgramState *state ) {
    memset( state, 0, sizeof( RunningProgramState ) );
    if ( is_running ) {
        state->is_program_running = true;
        state->program_index = current_program_index;
        state->zone_index = current_zone_index;
        state->zone_left_seconds =
                ( xTimerGetExpiryTime( timer ) - xTaskGetTickCount() ) * portTICK_PERIOD_MS / 1000 + 1;
    } else {
        state->is_program_running = false;
    }
}

int program_logic_get_queued_program( int queue_index ) {
    struct queue_item_t *item = queue;
    while ( item != NULL && queue_index-- > 0 ) {
        item = item->next;
    }
    return item != NULL ? item->program_index : -1;
}

void program_logic_pump_state_change( bool is_on ) {
    fire_command( COMMAND_PUMP_STATE, -1, is_on );
}

void program_logic_init() {
    is_running = false;
    current_program = NULL;

    event_queue = xQueueCreate( 16, sizeof( program_command_t ) );
    xTaskCreate( task_main, LOG_TAG, 3072, NULL, 10, NULL );

    timer = xTimerCreate(
            LOG_TAG,
            1,
            0,
            NULL,
            timer_callback );
    if ( !timer ) {
        ESP_LOGE( LOG_TAG, "Failed to create timer" );
    }
}
