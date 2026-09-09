#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "program.h"
#include "program_logic.h"
#include "program_start.h"
#include "sntp_main.h"

static const char *LOG_TAG = "program_starter";

static QueueHandle_t gpio_evt_queue = NULL;
static int last_check_time = -1;

static void timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    xQueueSend( gpio_evt_queue, &dummy, 0 );
}

static bool check_day_on( ProgramDay *day, struct tm *timeinfo ) {
    int week_day = ( timeinfo->tm_wday + 6 ) % 7;
    bool start = PROGRAM_ON_DAY( day->on_days, week_day ) != 0;
//    ESP_LOGI( LOG_TAG, "week day %d, day mask %02x, state %d", week_day, day->on_days, start );
    return start;
}

static bool check_day_interval( ProgramDay *day, struct tm *timeinfo ) {
    return false;
}

static bool check_day( Program *program, struct tm *timeinfo ) {
    if ( program->days.type == onDays ) {
        return check_day_on( &program->days, timeinfo );
    } else if ( program->days.type == interval ) {
        return check_day_interval( &program->days, timeinfo );
    }
    return false;
}

static void check_for_program_start() {
//    ESP_LOGI( LOG_TAG, "check_for_program_start" );
    if ( !sntp_is_time_set()) {
//        ESP_LOGW( LOG_TAG, "no time set yet" );
        return;
    }

    time_t now;
    time( &now );
    struct tm timeinfo;
    localtime_r( &now, &timeinfo );
    int current_time = PROGRAM_START_TIME( timeinfo.tm_hour, timeinfo.tm_min );
    if ( last_check_time == current_time ) {
        return;
    }
    last_check_time = current_time;

    int program_count = program_get_count();
    for ( int program_index = 0; program_index < program_count; program_index++ ) {
        Program *program = program_get( program_index );
        if ( !program->valid ) {
//            ESP_LOGW( LOG_TAG, "Program %d is invalid", program_index );
            continue;
        }
        if ( !program->valid || !program->enabled ) {
//            ESP_LOGI( LOG_TAG, "Program %d is not enabled", program_index );
            continue;
        }
        for ( int start_time_index = 0; start_time_index < program->start_times_count; start_time_index++ ) {
            int start_time = program->start_times[ start_time_index ];
            if ( start_time == current_time && check_day( program, &timeinfo )) {
                program_logic_start( program_index );
            }
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
    xTaskCreate( task_main, LOG_TAG, 3072, NULL, 10, NULL );

    TimerHandle_t timer = xTimerCreate(
            LOG_TAG,
            pdMS_TO_TICKS( 30 * 1000 ),
            1,
            NULL,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );
}
