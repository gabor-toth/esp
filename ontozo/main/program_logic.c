#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "lib/common.h"
#include "lib/gpio_define.h"
#include "gpio_logic.h"
#include "program_logic.h"
#include "program.h"

#define MAX_QUEUE_LENGTH 16

#define NOT_RUNNING -1

static const char *LOG_TAG = "program_logic";

static int running_program_index = NOT_RUNNING;
static int running_zone_index;
static Program *current_program;
static TimerHandle_t timer;
static int queue[MAX_QUEUE_LENGTH];
static int queue_length = 0;

static QueueHandle_t gpio_evt_queue = NULL;

static void start_next_zone() {
    if ( running_zone_index >= 0 ) {
        gpio_set_pin_state( OUTPUTS, ZONES_CLASS, current_program->zones[ running_zone_index ].zone_id, false );
    }
    if ( ++running_zone_index == current_program->zones_count ) {
        ESP_LOGI( LOG_TAG, "Ending program %d after last zone", running_program_index );
        program_logic_stop();
        return;
    }
    ProgramZone *zone = &current_program->zones[ running_zone_index ];
    ESP_LOGI( LOG_TAG, "moving to zone %d/%d: id %d, duration %d secs",
              running_zone_index, current_program->zones_count,
              zone->zone_id, zone->duration_in_seconds );
    gpio_set_pin_state( OUTPUTS, ZONES_CLASS, zone->zone_id, true );
    ESP_LOGI( LOG_TAG, "set timer to %d ticks", zone->duration_in_seconds * 1000 / portTICK_PERIOD_MS );
    xTimerChangePeriod( timer, zone->duration_in_seconds * 1000 / portTICK_PERIOD_MS, staticDONT_BLOCK );
    xTimerReset( timer, staticDONT_BLOCK );
}

static void timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    xQueueSend( gpio_evt_queue, &dummy, 0 );
}

_Noreturn static void task_main( void *arg ) {
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( gpio_evt_queue, &dummy, portMAX_DELAY )) {
            start_next_zone();
        }
    }
}

void program_logic_init() {
    running_program_index = NOT_RUNNING;
    current_program = NULL;

    gpio_evt_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( task_main, LOG_TAG, 2048, NULL, 10, NULL);

    timer = xTimerCreate(
            "program_runner",
            1,
            0,
            NULL,
            timer_callback );
}

static void program_logic_start( int index ) {
    if ( running_program_index >= 0 ) {
        ESP_LOGW( LOG_TAG, "Program %d is already running", running_program_index );
        return;
    }
    current_program = program_get( index );
    if ( current_program == NULL) {
        return;
    }
    ESP_LOGI( LOG_TAG, "Starting program %d", index );
    running_program_index = index;
    running_zone_index = -1;
    gpio_pump_main( true );
    start_next_zone();
}

void program_logic_queue_start( int index ) {
    if ( running_program_index == NOT_RUNNING ) {
        program_logic_start( index );
        return;
    }
    if ( queue_length >= MAX_QUEUE_LENGTH ) {
        return;
    }
    queue[ queue_length++ ] = index;
    ESP_LOGI( LOG_TAG, "Queued program %d", index );
}

void program_logic_move_to_next_zone() {
    start_next_zone();
}

void program_logic_get_state( RunningProgramState *state ) {
    memset( state, 0, sizeof( RunningProgramState ));
    if ( running_program_index >= 0 ) {
        state->is_program_running = true;
        state->program_index = running_program_index;
        state->zone_index = running_zone_index;
        state->zones_count = current_program->zones_count;
        state->zone_left_seconds =
                ( xTimerGetExpiryTime( timer ) - xTaskGetTickCount()) * portTICK_PERIOD_MS / 1000 + 1;
    } else {
        state->is_program_running = false;
    }
}

void program_logic_stop() {
    xTimerStop( timer, staticDONT_BLOCK );
    running_program_index = NOT_RUNNING;
    current_program = NULL;

    if ( queue_length > 0 ) {
        int index = queue[ 0 ];
        if ( --queue_length != 0 ) {
            memmove( queue, queue + 1, queue_length * sizeof( queue[ 0 ] ));
        }
        program_logic_start( index );
    } else {
        gpio_pump_main( false );
    }
}
