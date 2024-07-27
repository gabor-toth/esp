#include "debug_helper.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "n2k_gateway.h"
#include "n2k_gateway_actisense.h"
#include "n2k_gateway_wifi.h"
#include "n2k_simulator.h"
#include "sdkconfig.h"

static const char *TAG = "n2k-gw";

void debug_timer_cb( void *arg ) {
    debug_print_free_mem( nullptr );
}

void start_free_mem_timer() {
    esp_timer_create_args_t timer_args = {
            .callback = debug_timer_cb,
            .arg = nullptr,
            .dispatch_method= ESP_TIMER_TASK,
            .name = nullptr,
            .skip_unhandled_events= true,
    };
    esp_timer_handle_t timer_handle = nullptr;
    ESP_ERROR_CHECK( esp_timer_create( &timer_args, &timer_handle ) );
    ESP_ERROR_CHECK( esp_timer_start_periodic( timer_handle, 1000L * 1000L ) );
}

static uint16_t sentPackets = 0;

static void logCount( void *arg) {
    ESP_LOGI( TAG, "Sent %d packets", sentPackets );
    sentPackets = 0;
}

void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg ) {
    sentPackets++;
#if CONFIG_SIGNALK_OVER_ACTISENSE
    sendN2KMessageToSignalKOverActisense( N2kMsg );
#endif
#if CONFIG_SIGNALK_OVER_WIFI
    sendN2KMessageToSignalKOverWifi( N2kMsg );
#endif
}

void setupSignalkChannels() {
#if CONFIG_SIGNALK_LOG_FREE_MEM
    start_free_mem_timer();
#endif
#if CONFIG_SIGNALK_OVER_ACTISENSE
    setupSignalkOverActisense();
#endif
#if CONFIG_SIGNALK_OVER_WIFI
    setupSignalkOverWifi();
#endif
#if CONFIG_SIGNALK_START_SIMULATOR
    pngSimulationStart();
#endif
    esp_timer_create_args_t timer_args = {
            .callback = logCount,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = nullptr
    };
    esp_timer_handle_t log_timer = nullptr;
    ESP_ERROR_CHECK( esp_timer_create( &timer_args, &log_timer ) );
    ESP_ERROR_CHECK( esp_timer_start_periodic( log_timer, 1000L * 1000 ) );
}
