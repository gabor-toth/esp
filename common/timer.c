#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "string.h"
#include "timer.h"

static const char *TAG = "timer";

typedef struct {
    char *name;
    QueueHandle_t event_queue;
    timer_callback_t callback;
    void *user_data;
} TimerData;

_Noreturn static void timer_task_main( void *arg ) {
    TimerData *timerData = arg;
    
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( timerData->event_queue, &dummy, portMAX_DELAY ) ) {
            timerData->callback( timerData->user_data );
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    TimerData *timerData = pvTimerGetTimerID( timer );
    
    uint32_t dummy = 0;
    ESP_LOGI( TAG, "tick %s", timerData->name );
    xQueueSendToBack( timerData->event_queue, &dummy, 0 );
}

esp_err_t
timer_start( const char *name, timer_callback_t callback, int interval_ms, void *user_data, bool tickOnCreate ) {
    TimerData *timerData = calloc( sizeof( TimerData ), 1 );
    if ( timerData == NULL ) {
        return ESP_ERR_NO_MEM;
    }
    timerData->name = strdup( name );
    if ( timerData->name == NULL ) {
        return ESP_ERR_NO_MEM;
    }
    timerData->event_queue = xQueueCreate( 10, sizeof( uint32_t ) );
    if ( timerData->event_queue == 0 ) {
        ESP_LOGE( TAG, "Unable to create event queue" );
        return ESP_FAIL;
    }
    timerData->callback = callback;
    timerData->user_data = user_data;
    BaseType_t result = xTaskCreate( timer_task_main, TAG, 3072, timerData, 5, NULL );
    if ( pdPASS != result ) {
        ESP_LOGE( TAG, "Unable to create task with error %d", result );
        return ESP_FAIL;
    }
    
    TimerHandle_t timer = xTimerCreate(
            TAG,
            pdMS_TO_TICKS( interval_ms ),
            1,
            timerData,
            timer_callback );
    if ( timer == 0 ) {
        ESP_LOGE( TAG, "Unable to create timer" );
        return ESP_FAIL;
    }
    xTimerStart( timer, portMAX_DELAY );
    
    if ( tickOnCreate ) {
        timer_callback( timer );
    }
    return ESP_OK;
}
