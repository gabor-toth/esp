#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "n2k_sender.h"
#include "n2k_struct_parser.h"
#include <N2kTimer.h>
#include <vector>

using namespace std;

static const char *TAG = "n2k_sender";
typedef struct loopback_callback_node_t {
    struct loopback_callback_node_t* next;
    n2k_loopback_callback callback;
} loopback_callback_node_t;

static loopback_callback_node_t* loopback_callbacks = nullptr;

// Structure for holding message sending information
struct tN2kSendMessage {
    tN2kSendFunction SendFunction;
    const char *const Description;
    tN2kSyncScheduler Scheduler;

    tN2kSendMessage( tN2kSendFunction sendFunction,
                     const char *const description,
                     uint32_t /* nextTime */,
                     uint32_t period,
                     uint32_t offset,
                     bool enabled ) :
            SendFunction( sendFunction ),
            Description( description ),
            Scheduler( enabled, period, offset ) {}

    void Enable( bool state );
};

void tN2kSendMessage::Enable( bool state ) {
    if ( Scheduler.IsEnabled() != state ) {
        if ( state ) {
            Scheduler.UpdateNextTime();
        } else {
            Scheduler.Disable();
        }
    }
}

static vector<tN2kSendMessage> sendMessages;

static QueueHandle_t timer_event_queue = nullptr;

_Noreturn static void task_main( void *arg ) {
    (void) arg;

    for ( ;; ) {
        void *dummy;

        if ( !xQueueReceive( timer_event_queue, &dummy, portMAX_DELAY )) {
            continue;
        }

        vector<tN2kSendMessage>::iterator iterator;
        for ( iterator = sendMessages.begin(); iterator != sendMessages.end(); iterator++ ) {
            if ( !iterator->Scheduler.IsTime()) {
                continue;
            }
            iterator->Scheduler.UpdateNextTime();

            ESP_LOGD( TAG, "sending for %s", iterator->Description );
            int index;
            tN2kMsg N2kMsg;
            for ( index = 0; iterator->SendFunction( index, N2kMsg ); index++ ) {
                NMEA2000.SendMsg( N2kMsg );
                loopback_callback_node_t * node = loopback_callbacks;
                while ( node != nullptr ) {
                    node->callback( N2kMsg );
                    node=node->next;
                }
            }
            if ( index == 0 ) {
                ESP_LOGI( TAG, "nothing to send for %s", iterator->Description );
            }
        }
    }
}

static void timer_callback( TimerHandle_t ) {
    void *dummy = nullptr;
    xQueueSendToBack( timer_event_queue, &dummy, 0 );
}

void n2k_sender_on_open() {
    ESP_LOGI( TAG, "n2k_sender_on_open" );
    vector<tN2kSendMessage>::iterator iterator;
    for ( iterator = sendMessages.begin(); iterator != sendMessages.end(); iterator++ ) {
        if ( iterator->Scheduler.IsEnabled()) {
            ESP_LOGI( TAG, "starting scheduler %s", iterator->Description );
            iterator->Scheduler.UpdateNextTime();
        }
    }
}

void nk2_register_sender( tN2kSendFunction sendFunction,
                          const char *const description,
                          uint32_t periodMs,
                          uint32_t offsetMs,
                          bool enabled ) {
    tN2kSendMessage sendMessage( sendFunction, strdup( description ), 0, periodMs, offsetMs, enabled );
    sendMessages.push_back( sendMessage );

    if ( timer_event_queue == nullptr ) {
        timer_event_queue = xQueueCreate( 10, sizeof( void * ));
        xTaskCreate( task_main, TAG, 3072, nullptr, 10, nullptr );

        int interval_ms = 10; // >= 10ms
        TimerHandle_t timer = xTimerCreate(
                TAG,
                pdMS_TO_TICKS( interval_ms ),
                1,
                nullptr,
                timer_callback );
        xTimerStart( timer, portMAX_DELAY );
        ESP_LOGI( TAG, "timer started for %s with %dms interval", TAG, interval_ms );
    }
}

void n2k_sender_register_loopback( n2k_loopback_callback callback ) {
    loopback_callback_node_t * node = (loopback_callback_node_t*)malloc(sizeof(loopback_callback_node_t));
    node->next = loopback_callbacks;
    node->callback = callback;
    loopback_callbacks = node;
}
