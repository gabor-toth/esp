#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "lib/gpio_define.h"
#include "gpio_logic.h"
#include "program_logic.h"
#include "program.h"

/* A block time of 0 simply means "don't block". */
#define staticDONT_BLOCK                    ( ( TickType_t ) 0 )

static const char *LOG_TAG = "program_logic";

static int running_program_index = -1;
static int running_zone_index;
static Program *current_program;
static TimerHandle_t timer;

static void start_next_zone() {
    if ( running_zone_index >= 0 ) {
        gpio_set_pin_state( OUTPUTS, ZONES, current_program->zones[ running_zone_index ].zone_id, false );
    }
    if ( ++running_zone_index == current_program->zones_count ) {
        ESP_LOGI( LOG_TAG, "program ended after last zone" );
        program_logic_stop();
        return;
    }
    ProgramZone *zone = &current_program->zones[ running_zone_index ];
    ESP_LOGI( LOG_TAG, "moving to zone %d/%d: id %d, duration %d secs",
              running_zone_index, current_program->zones_count,
              zone->zone_id, zone->duration_in_seconds );
    gpio_set_pin_state( OUTPUTS, ZONES, zone->zone_id, true );
    ESP_LOGI( LOG_TAG, "set timer to %d ticks", zone->duration_in_seconds * 1000 / portTICK_PERIOD_MS );
    xTimerChangePeriod( timer, zone->duration_in_seconds * 1000 / portTICK_PERIOD_MS, staticDONT_BLOCK );
    xTimerReset( timer, staticDONT_BLOCK );
}

static void timer_callback( TimerHandle_t timer ) {
    start_next_zone();
}

void program_logic_init() {
    running_program_index = -1;
    current_program = NULL;

    timer = xTimerCreate(
            "program_runner",
            1,
            0,
            NULL,
            timer_callback );
}

void program_logic_start( int index ) {
    if ( running_program_index >= 0 ) {
        ESP_LOGW( LOG_TAG, "Program %d is already running", running_program_index );
        return;
    }
    current_program = program_get( index );
    if ( current_program == NULL) {
        return;
    }
    running_program_index = index;
    running_zone_index = -1;
    start_next_zone();
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
    running_program_index = -1;
    current_program = NULL;
}
