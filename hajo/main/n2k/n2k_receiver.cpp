#include "esp_event.h"
#include "esp_log.h"
#include "n2k_receiver.h"
#include "n2k_sender.h"
#include "lib/nvs_main.h"

using namespace std;

static const char *LOG = "n2k_recv";

_Noreturn static void task_main( void *arg ) {
    (void) arg;

    ESP_LOGI(LOG,"receive loop starting");
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
        ESP_LOGI( LOG, "Loaded address %02x", address );
        free( s );
    } else {
        ESP_LOGI( LOG, "No address set yet, using default %02x", address );
    }
    nvs_close_storage( nvs_handle );
    return address;
}

void n2k_save_address( uint8_t address ) {
    ESP_LOGI( LOG, "Save new address %02x", address );
    uint32_t nvs_handle = nvs_open_storage();
    char s[8];
    itoa( address, s, 10 );
    nvs_write_string( nvs_handle, "address", s );
    nvs_close_storage( nvs_handle );
}

static void n2k_on_open() {
    n2k_sender_on_open();
}

void n2k_init() {
    xTaskCreate( task_main, LOG, 3072, nullptr, 0, nullptr );

    uint8_t sourceAddress = n2k_load_address();
    NMEA2000.SetMode( tNMEA2000::N2km_ListenAndNode, sourceAddress );
    NMEA2000.SetOnOpen( n2k_on_open );
    NMEA2000.Open();
}
