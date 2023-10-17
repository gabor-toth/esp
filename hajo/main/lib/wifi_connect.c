#include "sdkconfig.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)
#include <string.h>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include <esp_log.h>
#include "wifi_connect.h"

// code copied from examples/common_components/protocol_examples_common/connect.c

static const char *TAG = "wifi";

static int s_active_interfaces = 0;
static SemaphoreHandle_t s_semph_get_ip_addrs;
static esp_netif_t *s_esp_netif = NULL;
static esp_ip4_addr_t s_ip_addr;

//#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (s_active_interfaces*2)
#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (s_active_interfaces)

static bool is_our_netif( const char *prefix, esp_netif_t *netif ) {
    return strncmp( prefix, esp_netif_get_desc( netif ), strlen( prefix ) - 1 ) == 0;
}

esp_netif_t *get_example_netif_from_desc( const char *desc ) {
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf( &expected_desc, "%s: %s", TAG, desc );
    while (( netif = esp_netif_next( netif )) != NULL) {
        if ( strcmp( esp_netif_get_desc( netif ), expected_desc ) == 0 ) {
            free( expected_desc );
            return netif;
        }
    }
    free( expected_desc );
    return netif;
}

static void on_got_ip( void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data ) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    if ( !is_our_netif( TAG, event->esp_netif )) {
        ESP_LOGW( TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc( event->esp_netif ));
        return;
    }
    ESP_LOGI( TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc( event->esp_netif ),
              IP2STR( &event->ip_info.ip ));
    memcpy( &s_ip_addr, &event->ip_info.ip, sizeof( s_ip_addr ));
    xSemaphoreGive( s_semph_get_ip_addrs );
}

static void on_wifi_disconnect( void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "Wi-Fi disconnected, trying to reconnect..." );
    esp_err_t err = esp_wifi_connect();
    if ( err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK( err );
}

#define DEFAULT_SCAN_LIST_SIZE 16

static char *wifi_ssid = NULL;
static char *wifi_password = NULL;

static void set_default_wifi() {
    wifi_ssid = CONFIG_EXAMPLE_WIFI_SSID;
    wifi_password = CONFIG_EXAMPLE_WIFI_PASSWORD;
}

static char *known_wifi_networks[][2] = {
        { "TothKiss", "ToThKiSs" },
        { "P92WG_E",  "Newmexicobrother08" },
        { "TGA",      "ToThKiSs01" },
};

static void find_known_wifi( wifi_ap_record_t *ap_info, uint16_t ap_count ) {
    for ( int i = 0; i < sizeof known_wifi_networks / sizeof known_wifi_networks[ 0 ]; i++ ) {
        if ( strcmp( known_wifi_networks[ i ][ 0 ], (char *) ap_info->ssid ) == 0 ) {
            wifi_ssid = known_wifi_networks[ i ][ 0 ];
            wifi_password = known_wifi_networks[ i ][ 1 ];
            return;
        }
    }
    set_default_wifi();
}

static void wifi_scan( void ) {
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert( sta_netif );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset( ap_info, 0, sizeof( ap_info ));

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ));
    ESP_ERROR_CHECK( esp_wifi_start());
    esp_wifi_scan_start(NULL, true );
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &number, ap_info ));
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &ap_count ));
    ESP_LOGI( TAG, "Total APs scanned = %u", ap_count );
    for ( int i = 0; ( i < DEFAULT_SCAN_LIST_SIZE ) && ( i < ap_count ); i++ ) {
        ESP_LOGI( TAG, "SSID %s", ap_info[ i ].ssid );
    }

    find_known_wifi( ap_info, ap_count );

    esp_wifi_stop();
    esp_netif_destroy_default_wifi( sta_netif );
}

static esp_netif_t *wifi_start( void ) {
    wifi_scan();

    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf( &desc, "%s: %s", TAG, esp_netif_config.if_desc );
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi( WIFI_IF_STA, &esp_netif_config );
    free( desc );
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL ));
    ESP_ERROR_CHECK( esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL ));

    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ));
    wifi_config_t wifi_config = {
            .sta = {
                    .scan_method = WIFI_ALL_CHANNEL_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
    };
    strncpy((char *) wifi_config.sta.ssid, wifi_ssid, sizeof wifi_config.sta.ssid );
    strncpy((char *) wifi_config.sta.password, wifi_password, sizeof wifi_config.sta.password );

    ESP_LOGI( TAG, "Connecting to %s...", wifi_config.sta.ssid );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ));
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifi_config ));
    ESP_ERROR_CHECK( esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

static void wifi_stop( void ) {
    ESP_ERROR_CHECK( esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect ));
    ESP_ERROR_CHECK( esp_event_handler_unregister( IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip ));
    esp_err_t err = esp_wifi_stop();
    if ( err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK( esp_wifi_deinit());
    ESP_ERROR_CHECK( esp_wifi_clear_default_wifi_driver_and_handlers( s_esp_netif ));
    esp_netif_destroy( s_esp_netif );
    s_esp_netif = NULL;
}

static void start( void ) {
    s_esp_netif = wifi_start();
    s_active_interfaces++;
    /* create semaphore if at least one interface is active */
    s_semph_get_ip_addrs = xSemaphoreCreateCounting( NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0 );
}

/* tear down connection, release resources */
static void stop( void ) {
    wifi_stop();
    s_active_interfaces--;
}


esp_err_t wifi_connect( void ) {
#if EXAMPLE_DO_CONNECT
    if (s_semph_get_ip_addrs != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
#endif
    start();
    ESP_ERROR_CHECK( esp_register_shutdown_handler( &stop ));
    ESP_LOGI( TAG, "Waiting for IP(s)" );
    for ( int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i ) {
        xSemaphoreTake( s_semph_get_ip_addrs, portMAX_DELAY );
    }
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    for ( int i = 0; i < esp_netif_get_nr_of_ifs(); ++i ) {
        netif = esp_netif_next( netif );
        if ( is_our_netif( TAG, netif )) {
            ESP_LOGI( TAG, "Connected to %s", esp_netif_get_desc( netif ));
            ESP_ERROR_CHECK( esp_netif_get_ip_info( netif, &ip ));

            ESP_LOGI( TAG, "- IPv4 address: " IPSTR, IP2STR( &ip.ip ));
        }
    }
    return ESP_OK;
}

#endif
