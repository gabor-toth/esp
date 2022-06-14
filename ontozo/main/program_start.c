#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "lib/sntp_main.h"
#include "program.h"
#include "program_start.h"

static const char *LOG_TAG = "program_starter";

static QueueHandle_t gpio_evt_queue = NULL;

static void timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    xQueueSend( gpio_evt_queue, &dummy, 0 );
}

static void check_for_program_start() {
    ESP_LOGI( LOG_TAG, "check_for_program_start" );
    if ( !sntp_is_time_set()) {
        return;
    }

    int program_count = program_get_count();
    for ( int i = 0; i < program_count; i++ ) {
        Program *program = program_get( i );
        if ( !program->valid || !program->enabled ) {
            continue;
        }
    }
}

_Noreturn static void task_main( void *arg ) {
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( gpio_evt_queue, &dummy, portMAX_DELAY )) {
            check_for_program_start();
        }
    }
}

void program_start_init() {
    check_for_program_start();

    gpio_evt_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( task_main, LOG_TAG, 2048, NULL, 10, NULL);

    xTimerCreate(
            LOG_TAG,
            30 * 1000 / portTICK_PERIOD_MS,
            1,
            NULL,
            timer_callback );

}
