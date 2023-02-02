#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "n2k_sender.h"
#include "n2k_protocol.h"
#include "n2k_png.h"
#include <string.h>

static const char *LOG = "n2k_sender";
static QueueHandle_t timer_event_queue = NULL;

typedef struct {
    char *name;
    n2k_sender_callback callback;
} callback_data_t;

_Noreturn static void task_main( void *arg ) {
    (void) arg;

    for ( ;; ) {
        callback_data_t *timer_data;
        if ( xQueueReceive( timer_event_queue, &timer_data, portMAX_DELAY )) {
            can_message_t can_message;
            int index = 0;
            for ( index = 0; timer_data->callback( index, &can_message ); index++ ) {
//                ESP_LOGI( LOG, "sending %s/%d", timer_data->name, index );
                n2k_send( &can_message );
            }
            if ( index == 0 ) {
                ESP_LOGI( LOG, "nothing to send for %s", timer_data->name );
            }
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    callback_data_t *timer_data = pvTimerGetTimerID( timer );
//    ESP_LOGI( LOG, "tick %s", timer_data->name );
    xQueueSend( timer_event_queue, &timer_data, 0 );
}

void nk2_register_sender( const char *name, int interval_ms, n2k_sender_callback callback ) {
    if ( timer_event_queue == NULL) {
        timer_event_queue = xQueueCreate( 10, sizeof( callback_data_t * ));
        xTaskCreate( task_main, LOG, 3072, NULL, 10, NULL);
    }

    callback_data_t *timer_data = malloc( sizeof( callback_data_t ));
    timer_data->name = strdup( name );
    timer_data->callback = callback;

    TimerHandle_t timer = xTimerCreate(
            name,
            pdMS_TO_TICKS( interval_ms ),
            1,
            timer_data,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );

    ESP_LOGI( LOG, "timer started for %s with %dms interval", name, interval_ms );
}
