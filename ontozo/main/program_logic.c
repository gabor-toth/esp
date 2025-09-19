#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "gpio_define.h"
#include "gpio_logic.h"
#include "program_logic.h"
#include "program.h"

static const char *LOG_TAG = "program_logic";

struct queue_item_t {
    int64_t program_id;
    int program_index;
    int zones_count;
    bool *zones_disabled;
    struct queue_item_t *next;
};

static bool is_running = false;
static int current_program_index;
static int current_zone_id = -1;
static int current_zone_index;
static Program *current_program;
static int64_t current_program_id;
static TimerHandle_t timer;
static struct queue_item_t *queue = NULL;

static SemaphoreHandle_t semaphore;
static StaticSemaphore_t xMutexBuffer;

static void stop_and_move_to_next_program();

static void destruct_queue_item( struct queue_item_t *item ) {
    if ( item->zones_disabled ) {
        free( item->zones_disabled );
    }
    free( item );
}

static void end_current_zone() {
    if ( current_zone_id >= 0 ) {
        gpio_set_pin_state( OUTPUTS, ZONES_CLASS, current_zone_id, false );
        current_zone_id = -1;
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
    current_zone_id = zone->zone_id;
    ESP_LOGI( LOG_TAG, "moving to zone %d/%d: id %d, duration %d secs",
              current_zone_index + 1, current_program->zones_count, current_zone_id, zone->duration_in_seconds );
    gpio_set_pin_state( OUTPUTS, ZONES_CLASS, current_zone_id, true );
    TickType_t timer_ticks = pdMS_TO_TICKS( zone->duration_in_seconds * 1000 );
    ESP_LOGI( LOG_TAG, "set timer to %ld ticks", timer_ticks );
    if ( !xTimerChangePeriod( timer, timer_ticks, portMAX_DELAY ) ) {
        ESP_LOGE( LOG_TAG, "xTimerChangePeriod failed, aborting program" );
    }
}

static void timer_callback( TimerHandle_t unused ) {
    xSemaphoreTake( semaphore, 1 );
    start_next_zone();
    xSemaphoreGive( semaphore );
}

static void start_program() {
    int index = queue->program_index;
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

static int64_t generate_program_id() {
    time_t id;
    time( &id );
    // TODO ensure unique id within 1 second
    return id;
}

static void start_or_queue_program( int program_index ) {
    struct queue_item_t *item = calloc( 1, sizeof( struct queue_item_t ) );
    Program *program = program_get( program_index );
    item->program_id = generate_program_id();
    item->program_index = program_index;
    item->zones_count = program->zones_count;
    item->zones_disabled = calloc( item->zones_count, sizeof( bool ) );
    if ( queue == NULL ) {
        queue = item;
        start_program();
    } else {
        struct queue_item_t *next = queue;
        while ( next->next != NULL ) {
            next = next->next;
        }
        next->next = item;
    }
    ESP_LOGI( LOG_TAG, "Queued program %d", program_index );
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
    struct queue_item_t *prev = queue;
    queue = queue->next;
    destruct_queue_item( prev );
    if ( queue != NULL ) {
        start_program();
    }
}

static void stop_all_programs() {
    while ( queue != NULL ) {
        struct queue_item_t *next = queue->next;
        destruct_queue_item( queue );
        queue = next;
    }
    stop_and_move_to_next_program();
}

static void pump_state_changed( bool is_on ) {
    // TODO add logic
}

static struct queue_item_t *find_program_by_id( program_id_t program_id, struct queue_item_t **prev_item ) {
    struct queue_item_t *prev = NULL;
    struct queue_item_t *item = queue;
    while ( item != NULL && item->program_id != program_id ) {
        prev = item;
        item = item->next;
    }
    if ( prev_item != NULL ) {
        *prev_item = prev;
    }
    return item;
}

void program_logic_start( int program_index ) {
    xSemaphoreTake( semaphore, 10 );
    start_or_queue_program( program_index );
    xSemaphoreGive( semaphore );
}

void program_logic_move_to_next_zone( program_id_t program_id, int zone_index ) {
    xSemaphoreTake( semaphore, 10 );
    if ( program_id == 0 || ( current_program_id == program_id && zone_index == current_zone_index ) ) {
        start_next_zone();
    }
    xSemaphoreGive( semaphore );
}

void program_logic_move_to_next_program( program_id_t program_id ) {
    xSemaphoreTake( semaphore, 10 );
    if ( program_id == 0 || current_program_id == program_id ) {
        stop_and_move_to_next_program();
    }
    xSemaphoreGive( semaphore );
}

void program_logic_stop_all() {
    xSemaphoreTake( semaphore, 10 );
    stop_all_programs();
    xSemaphoreGive( semaphore );
}

void program_logic_cancel_scheduled_program( program_id_t program_id ) {
    xSemaphoreTake( semaphore, 10 );
    struct queue_item_t *prev;
    struct queue_item_t *item = find_program_by_id( program_id, &prev );
    if ( item != NULL ) {
        if ( prev != NULL ) {
            prev->next = item->next;
        } else {
            queue = item->next;
        }
        destruct_queue_item( item );
        ESP_LOGI( LOG_TAG, "Program %lld cancelled", program_id );
    } else {
        ESP_LOGW( LOG_TAG, "No program with id %lld", program_id );
    }
    xSemaphoreGive( semaphore );
}

void program_logic_toggle_scheduled_zone( program_id_t program_id, int zone_index ) {
    xSemaphoreTake( semaphore, 10 );
    struct queue_item_t *item = find_program_by_id( program_id, NULL );
    if ( item != NULL ) {
        if ( zone_index < item->zones_count ) {
            if ( queue == item && zone_index <= current_zone_index ) {
                ESP_LOGW( LOG_TAG, "Program %lld zone %d is active or past, not changing", program_id, zone_index );
            } else {
                item->zones_disabled[ zone_index ] = !item->zones_disabled[ zone_index ];
            }
            ESP_LOGI( LOG_TAG, "Program %lld zone %d set to %s", program_id, zone_index,
                      item->zones_disabled[ zone_index ] ? "disabled" : "enabled" );
        } else {
            ESP_LOGW( LOG_TAG, "Program %lld zone %d is out of range (>= %d)", program_id, zone_index,
                      item->zones_count );
        }
    } else {
        ESP_LOGW( LOG_TAG, "No program with id %lld", program_id );
    }
    xSemaphoreGive( semaphore );
}

int program_logic_get_queued_programs( RunningProgramState *running_state, QueuedProgramState **queue_state ) {
    *queue_state = NULL;
    memset( running_state, 0, sizeof( RunningProgramState ) );
    xSemaphoreTake( semaphore, 10 );
    if ( is_running ) {
        running_state->is_program_running = true;
        running_state->program_index = current_program_index;
        running_state->zone_index = current_zone_index;
        running_state->zone_left_seconds =
                ( xTimerGetExpiryTime( timer ) - xTaskGetTickCount() ) * portTICK_PERIOD_MS / 1000 + 1;
    } else {
        running_state->is_program_running = false;
    }

    int count = 0;
    for ( struct queue_item_t *item = queue; item != NULL; item = item->next ) {
        count++;
    }
    *queue_state = calloc( count, sizeof( QueuedProgramState ) );
    int i = 0;
    for ( struct queue_item_t *item = queue; item != NULL; item = item->next, i++ ) {
        QueuedProgramState *result_item = *queue_state + i;
        result_item->program_id = item->program_id;
        result_item->program_index = item->program_index;
        result_item->zones_count = item->zones_count;
        result_item->zones_disabled = item->zones_disabled;
    }
    xSemaphoreGive( semaphore );
    return count;
}

void program_logic_pump_state_change( bool is_on ) {
    xSemaphoreTake( semaphore, 10 );
    pump_state_changed( is_on );
    xSemaphoreGive( semaphore );
}

bool program_logic_is_program_in_use( int program_index ) {
    xSemaphoreTake( semaphore, 10 );
    bool is_in_use = false;
    struct queue_item_t *item = queue;
    while ( item != NULL && !is_in_use ) {
        is_in_use = item->program_index == program_index;
        item = item->next;
    }
    xSemaphoreGive( semaphore );
    return is_in_use;
}

void program_logic_init() {
    is_running = false;
    current_program = NULL;

    semaphore = xSemaphoreCreateMutexStatic( &xMutexBuffer );
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
