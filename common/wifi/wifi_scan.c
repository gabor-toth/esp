#include "wifi_scan.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_main.h"
#include "wifi_main.h"

#define ASYNC_WIFI_INIT 1
#define DEFAULT_SCAN_LIST_SIZE 16
#define NVS_KEY_WIFI_HOSTNAME "wifi.hostname"

static const char *TAG = "wifi_scan";

typedef struct {
    char *ssid;
    char *password;
} known_wifi_network_t;

static known_wifi_network_t known_wifi_networks[] = {
        { .ssid = "TothKiss", .password = "ToThKiSs" },
        { .ssid ="P92WG_E", .password ="22Dailymuffintime77" },
        { .ssid ="TGA", .password ="ToThKiSs01" },
        { .ssid ="DIGI-02300875", .password ="qnZFucU6" },
};

static int selected_network_index;
//static int selected_channel;

static void handler_on_wifi_scan_done( void *sta_netif, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data );

// from connect.c
extern bool example_is_our_netif( const char *prefix, esp_netif_t *netif );

// from wifi_connect.c
extern esp_err_t example_wifi_connect( void );

// from wifi_connect.c
extern esp_netif_t *s_example_sta_netif;

static void wifi_scan( void ) {
#if !ASYNC_WIFI_INIT
#error "Use wifi_scan from wifi_connect.c"
#endif
    ESP_ERROR_CHECK( esp_netif_init() );
    // OWN commented out
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert( sta_netif );
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &handler_on_wifi_scan_done,
                                                 sta_netif ) );
    esp_wifi_scan_start( NULL, false );
}

static void find_known_wifi( wifi_ap_record_t *ap_info, uint16_t ap_count ) {
    int8_t best_rssi = INT8_MIN;
    selected_network_index = -1;
    
    for ( int ap = 0; ap < ap_count; ap++ ) {
        ESP_LOGI( TAG, "SSID %-16s channel %2d signal %3ddB", ap_info[ ap ].ssid, ap_info[ ap ].primary,
                  ap_info[ ap ].rssi );
        for ( int i = 0; i < sizeof known_wifi_networks / sizeof known_wifi_networks[ 0 ]; i++ ) {
            if ( strcmp( known_wifi_networks[ i ].ssid, (char *) ap_info[ ap ].ssid ) == 0 &&
                 ap_info[ ap ].rssi > best_rssi ) {
                best_rssi = ap_info[ ap ].rssi;
                selected_network_index = i;
//                selected_channel = ap_info[ ap ].primary;
            }
        }
    }
}

static void handler_on_wifi_scan_done( void *sta_netif, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data ) {
    (void) event_base;
    (void) event_id;
    
    wifi_event_sta_scan_done_t *scan_done_data = (wifi_event_sta_scan_done_t *) event_data;
    ESP_LOGI( TAG, "Scan finished with status %ld, number of results %d", scan_done_data->status,
              scan_done_data->number );
    
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &ap_count ) );
    wifi_ap_record_t *ap_info = calloc( ap_count, sizeof( wifi_ap_record_t ) );
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &ap_count, ap_info ) );
    find_known_wifi( ap_info, ap_count );
    free( ap_info );
    
    ESP_ERROR_CHECK(
            esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &handler_on_wifi_scan_done ) );
    
    esp_wifi_stop();
    esp_netif_destroy_default_wifi( (esp_netif_t *) sta_netif );
    
    if ( selected_network_index == -1 ) {
        ESP_LOGW( TAG, "No known wifi network found" );
        return;
    }
    example_wifi_connect();
}

esp_err_t wifi_connect( void ) {
    selected_network_index = -1;
    wifi_scan();
#if ASYNC_WIFI_INIT
    return ESP_OK;
#else
    return example_wifi_connect();
#endif
}

esp_netif_t *wifi_get_esp_netif() {
    return s_example_sta_netif;
}

#define EFUSE_MAC_BYTES 6

static char *get_generated_hostname() {
    char *hostname;
    int buf_size = strlen( CONFIG_OWN_CONFIG_HOST_NAME ) + 1;
#if CONFIG_OWN_CONFIG_HOST_NAME_SUFFIX_WITH_CHIPID
    uint8_t chipid[EFUSE_MAC_BYTES];
    buf_size += sizeof(chipid) * 2;
#endif
    hostname = malloc( buf_size );
    strcpy( hostname, CONFIG_OWN_CONFIG_HOST_NAME );
#if CONFIG_OWN_CONFIG_HOST_NAME_SUFFIX_WITH_CHIPID
    char *end = strchr( hostname, '\0' );
    *(end++) = '_';
    esp_efuse_mac_get_default( chipid );
    for ( int i = 0; i < sizeof(chipid); i++, end += 2 ) {
        sprintf( end, "%02x", chipid[ i ] );
    }
#endif
    return hostname;
}

static char *get_stored_hostname() {
    uint32_t nvs_handle = nvs_open_storage();
    char *s = nvs_read_string( nvs_handle, NVS_KEY_WIFI_HOSTNAME );
    nvs_close_storage( nvs_handle );
    return s;
}

void wifi_set_hostname() {
    char *hostname;
    hostname = get_stored_hostname();
    if ( hostname == NULL ) {
        hostname = get_generated_hostname();
    }
    ESP_LOGI( TAG, "hostname is %s", hostname );
    ESP_ERROR_CHECK( esp_netif_set_hostname( s_example_sta_netif, hostname ) );
    free( hostname );
}

void wifi_set_credentials( wifi_config_t *wifi_config ) {
    strncpy( (char *) wifi_config->sta.ssid, known_wifi_networks[ selected_network_index ].ssid,
             sizeof(wifi_config->sta.ssid) );
    strncpy( (char *) wifi_config->sta.password, known_wifi_networks[ selected_network_index ].password,
             sizeof(wifi_config->sta.password) );
}

bool wifi_wait_on_connect() {
    return !ASYNC_WIFI_INIT;
}