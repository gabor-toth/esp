#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "lib/nvs_main.h"
#include "n2k_receiver.h"
#include "n2k_sender.h"
#include "NMEA2000_esp32.h"
#include <stdatomic.h>

static const char *TAG = "n2k_recv";

static QueueHandle_t event_queue = nullptr;
static TimerHandle_t timer;

static int minimal_check_interval = 1000;
static std::atomic_int eventCounter = 0;

static void timer_callback(const char* reason) {
    if ( timer != nullptr) {
        assert(xTimerStop(timer, 0) == pdPASS );
    }
    int eventSerial = eventCounter++;
    ESP_LOGI(TAG,"event %d %s", eventSerial, reason);
    xQueueSendToBack( event_queue, &eventSerial, 0 );
}

static void timer_callback( TimerHandle_t ) {
    timer_callback("timer_callback");
}

void startTimer( TickType_t ticks ) {
    ESP_LOGI(TAG,"Start timer %p with %ld ticks", timer, ticks);
    assert( xTimerChangePeriod(timer, ticks, 0 ) == pdPASS ) ;
    assert( xTimerStart( timer, 0 )== pdPASS );
}

_Noreturn static void task_main_event( void *arg ) {
    (void) arg;

    ESP_LOGI(TAG,"receive loop starting");
    for ( ;; ) {
        ESP_LOGI(TAG,"ParseMessages");
        NMEA2000.ParseMessages();

        tN2kSchedulerTime next = NMEA2000.GetTimeOfNextEvent();
        TickType_t ticks;
        if ( next == 0 ) {
            ticks = 0;
        } else {
            long sleep = next == N2kSchedulerDisabled ? minimal_check_interval : (long) ( next - N2kMillis64());
            ticks = sleep <= 0 ? 0 : pdMS_TO_TICKS( sleep );
        }
        if ( ticks == 0 ) {
            continue;
        }

        ESP_LOGI(TAG,"sleeping %ld ticks", ticks);
        startTimer(ticks);

        int eventSerial;
        if ( !xQueueReceive( event_queue, &eventSerial, portMAX_DELAY )) {
            continue;
        }
        ESP_LOGI(TAG,"got event %d", eventSerial);
    }
}

_Noreturn static void task_main_pull( void *arg ) {
    (void) arg;

    ESP_LOGI(TAG,"receive loop starting");
    for ( ;; ) {
        // TODO make this interrupt-driven
        NMEA2000.ParseMessages();
    }
}

uint8_t n2k_load_address() {
    uint32_t nvs_handle = nvs_open_storage();
    char *s = nvs_read_string( nvs_handle, "address" );
    uint address = 25;
    if ( s != nullptr ) {
        address = atoi( s );
        ESP_LOGI( TAG, "Loaded address %02x", address );
        free( s );
    } else {
        ESP_LOGI( TAG, "No address set yet, using default %02x", address );
    }
    nvs_close_storage( nvs_handle );
    return address;
}

void n2k_save_address( uint8_t address ) {
    ESP_LOGI( TAG, "Save new address %02x", address );
    uint32_t nvs_handle = nvs_open_storage();
    char s[8];
    itoa( address, s, 10 );
    nvs_write_string( nvs_handle, "address", s );
    nvs_close_storage( nvs_handle );
}

static void n2k_on_open() {
    n2k_sender_on_open();
}

void n2k_wake_receiver() {
    timer_callback( "n2k_wake_receiver");
}

void n2k_init() {
    event_queue = xQueueCreate( 20, sizeof( int ));
    timer = xTimerCreate(
            TAG,
            portMAX_DELAY,
            0,
            nullptr,
            timer_callback );
    ESP_LOGI(TAG,"Timer %p created", timer);
    xTaskCreate( task_main_pull, TAG, 3072, nullptr, tskIDLE_PRIORITY, nullptr );
    //n2k_wake_receiver();

    uint8_t sourceAddress = n2k_load_address();
    NMEA2000.SetMode( tNMEA2000::N2km_ListenAndNode, sourceAddress );
    NMEA2000.SetOnOpen( n2k_on_open );
    NMEA2000.Open();
}
