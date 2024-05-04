#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "string.h"
#include "wifi_main.h"

// functions in other esp files

// in scan.c
extern void wifi_scan( void );

// in wifi_main.c
extern void example_wifi_shutdown( void );

extern esp_err_t example_wifi_connect( void );

// own

#define DEFAULT_SCAN_LIST_SIZE 16

static const char* TAG = "wifi_own";

typedef struct {
    char* ssid;
    char* password;
} known_wifi_network_t;

static known_wifi_network_t known_wifi_networks[] = {
        { .ssid = "sol", .password = "SoL37695" },
//        { .ssid = "TothKiss", .password = "ToThKiSs" },
//        { .ssid ="P92WG_E", .password ="22Dailymuffintime77" },
        { .ssid ="TGA", .password ="ToThKiSs01" },
//        { .ssid ="DIGI-02300875", .password ="qnZFucU6" },
};

static int selected_network_index;
static int selected_channel;

void wifi_scan_get_ssid_and_password( wifi_sta_config_t* wifi_config_sta ) {
    strncpy((char*) wifi_config_sta->ssid, known_wifi_networks[ selected_network_index ].ssid,
            sizeof(wifi_config_sta->ssid));
    strncpy((char*) wifi_config_sta->password, known_wifi_networks[ selected_network_index ].password,
            sizeof(wifi_config_sta->password));
//    wifi_config_sta->channel = selected_channel;
}

void wifi_set_hostname( esp_netif_t* netif ) {
    // TODO hostname should be read from flash
    ESP_ERROR_CHECK( esp_netif_set_hostname( netif, "sol-n2kgw" ));
}

void wifi_scan_start() {
    selected_network_index = -1;
    esp_wifi_scan_start(NULL, false );
}

static void find_known_wifi( wifi_ap_record_t* ap_info, uint16_t ap_count ) {
    int8_t best_rssi = INT8_MIN;
    selected_network_index = -1;

    for ( int ap = 0; ap < ap_count; ap++ ) {
        ESP_LOGI( TAG, "SSID %-16s channel %2d signal %3ddB", ap_info[ ap ].ssid, ap_info[ ap ].primary,
                  ap_info[ ap ].rssi );
        for ( int i = 0; i < sizeof known_wifi_networks / sizeof known_wifi_networks[ 0 ]; i++ ) {
            if ( strcmp( known_wifi_networks[ i ].ssid, (char*) ap_info[ ap ].ssid ) == 0 &&
                 ap_info[ ap ].rssi > best_rssi ) {
                best_rssi = ap_info[ ap ].rssi;
                selected_network_index = i;
                selected_channel = ap_info[ ap ].primary;
            }
        }
    }
}

void wifi_handler_on_scan_done( void* sta_netif, esp_event_base_t event_base,
                                int32_t event_id, void* event_data ) {
    wifi_event_sta_scan_done_t* scan_done_data = (wifi_event_sta_scan_done_t*) event_data;
    ESP_LOGI( TAG, "Scan finished with status %ld, number of results %d", scan_done_data->status,
              scan_done_data->number );

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &ap_count ));
    wifi_ap_record_t* ap_info = calloc( ap_count, sizeof( wifi_ap_record_t ));
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &ap_count, ap_info ));
    find_known_wifi( ap_info, ap_count );
    free( ap_info );

    ESP_ERROR_CHECK(
            esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_handler_on_scan_done ));

    esp_wifi_stop();
    esp_netif_destroy_default_wifi((esp_netif_t*) sta_netif );

    if ( selected_network_index == -1 ) {
        ESP_LOGW( TAG, "No known Wifi network found" );
//      TODO  timer
        return;
    }
    example_wifi_connect();
}

void wifi_shutdown( void ) {
    example_wifi_shutdown();
}

esp_err_t wifi_main( void ) {
    wifi_scan();
#if ASYNC_WIFI_INIT
    return ESP_OK;
#else
    return example_wifi_connect();
#endif
}