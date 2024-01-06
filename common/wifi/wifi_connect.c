#include "sdkconfig.h"

#include <string.h>
/* own commented out
#include "protocol_examples_common.h"
#include "example_common_private.h"
*/
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "wifi_main.h"

#if CONFIG_EXAMPLE_CONNECT_WIFI

// OWN see esp-idf/examples/common_components/protocol_examples_common/wifi_connect.c

static const char *TAG = "wifi_connect";
static esp_netif_t *s_example_sta_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
#if CONFIG_EXAMPLE_CONNECT_IPV6
static SemaphoreHandle_t s_semph_get_ip6_addrs = NULL;
#endif

/* OWN commented out
#if CONFIG_EXAMPLE_WIFI_SCAN_METHOD_FAST
#define EXAMPLE_WIFI_SCAN_METHOD WIFI_FAST_SCAN
#elif CONFIG_EXAMPLE_WIFI_SCAN_METHOD_ALL_CHANNEL
#define EXAMPLE_WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#endif

#if CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SIGNAL
#define EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SECURITY
#define EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#endif

#if CONFIG_EXAMPLE_WIFI_AUTH_OPEN
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_EXAMPLE_WIFI_AUTH_WEP
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA2_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA_WPA2_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA2_ENTERPRISE
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_ENTERPRISE
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA3_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_EXAMPLE_WIFI_AUTH_WPA2_WPA3_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_EXAMPLE_WIFI_AUTH_WAPI_PSK
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif
*/

static int s_retry_num = 0;
#define EXAMPLE_NETIF_DESC_STA    "mywifi"

// OWN start

// Wifi scan

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
static int selected_channel;

static void wifi_scan( void );

static void handler_on_wifi_scan_done( void *sta_netif, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data );

// own hostname
static void set_hostname();

// is in connect.c
extern bool example_is_our_netif( const char *prefix, esp_netif_t *netif );

#define ASYNC_WIFI_INIT 1

// OWN end

static void example_handler_on_wifi_disconnect( void *dummy, esp_event_base_t event_base,
                                                int32_t event_id, void *event_data ) {
    s_retry_num++;
    if ( s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY ) {
        ESP_LOGI( TAG, "Wifi Connect failed %d times, stop reconnect.", s_retry_num );
        /* let example_wifi_sta_do_connect() return */
        if ( s_semph_get_ip_addrs ) {
            xSemaphoreGive( s_semph_get_ip_addrs );
        }
#if CONFIG_EXAMPLE_CONNECT_IPV6
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        }
#endif
        return;
    }
    ESP_LOGI( TAG, "Wifi disconnected, trying to reconnect..." );
    esp_err_t err = esp_wifi_connect();
    if ( err == ESP_ERR_WIFI_NOT_STARTED ) {
        return;
    }
    ESP_ERROR_CHECK( err );
}

static void example_handler_on_wifi_connect( void *esp_netif, esp_event_base_t event_base,
                                             int32_t event_id, void *event_data ) {
    // OWN TODO logging fails with a strange error
//    ESP_LOGI( TAG, "Wifi connected" );
#if CONFIG_EXAMPLE_CONNECT_IPV6
    esp_netif_create_ip6_linklocal(esp_netif);
#endif // CONFIG_EXAMPLE_CONNECT_IPV6
}

static void example_handler_on_sta_got_ip( void *dummy, esp_event_base_t event_base,
                                           int32_t event_id, void *event_data ) {
    s_retry_num = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    if ( !example_is_our_netif( EXAMPLE_NETIF_DESC_STA, event->esp_netif ) ) {
        ESP_LOGI( TAG, "Got IPv4 event: wrong interface \"%s\" (%s) address: " IPSTR,
                  esp_netif_get_desc( event->esp_netif ),
                  EXAMPLE_NETIF_DESC_STA, IP2STR( &event->ip_info.ip ) );
        return;
    }
    ESP_LOGI( TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc( event->esp_netif ),
              IP2STR( &event->ip_info.ip ) );
    if ( s_semph_get_ip_addrs ) {
        xSemaphoreGive( s_semph_get_ip_addrs );
    } else {
        ESP_LOGI( TAG, "- IPv4 address: " IPSTR ",", IP2STR( &event->ip_info.ip ) );
    }
}

#if CONFIG_EXAMPLE_CONNECT_IPV6
static void example_handler_on_sta_got_ipv6(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    if (!example_is_our_netif(EXAMPLE_NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGI(TAG, "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s", esp_netif_get_desc(event->esp_netif),
             IPV62STR(event->ip6_info.ip), example_ipv6_addr_types_to_str[ipv6_type]);

    if (ipv6_type == EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE) {
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        } else {
            ESP_LOGI(TAG, "- IPv6 address: " IPV6STR ", type: %s", IPV62STR(event->ip6_info.ip), example_ipv6_addr_types_to_str[ipv6_type]);
        }
    }
}
#endif // CONFIG_EXAMPLE_CONNECT_IPV6

void example_wifi_start( void ) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    esp_netif_config.if_desc = EXAMPLE_NETIF_DESC_STA;
    esp_netif_config.route_prio = 128;
    s_example_sta_netif = esp_netif_create_wifi( WIFI_IF_STA, &esp_netif_config );
    // OWN added
    set_hostname();
    esp_wifi_set_default_wifi_sta_handlers();
    
    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ) );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void example_wifi_stop( void ) {
    esp_err_t err = esp_wifi_stop();
    if ( err == ESP_ERR_WIFI_NOT_INIT ) {
        return;
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK( esp_wifi_deinit() );
    ESP_ERROR_CHECK( esp_wifi_clear_default_wifi_driver_and_handlers( s_example_sta_netif ) );
    esp_netif_destroy( s_example_sta_netif );
    s_example_sta_netif = NULL;
}

esp_err_t example_wifi_sta_do_connect( wifi_config_t wifi_config, bool wait ) {
    if ( wait ) {
        s_semph_get_ip_addrs = xSemaphoreCreateBinary();
        if ( s_semph_get_ip_addrs == NULL ) {
            return ESP_ERR_NO_MEM;
        }
#if CONFIG_EXAMPLE_CONNECT_IPV6
        s_semph_get_ip6_addrs = xSemaphoreCreateBinary();
        if (s_semph_get_ip6_addrs == NULL) {
            vSemaphoreDelete(s_semph_get_ip_addrs);
            return ESP_ERR_NO_MEM;
        }
#endif
    }
    s_retry_num = 0;
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &example_handler_on_wifi_disconnect,
                                        NULL ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &example_handler_on_sta_got_ip, NULL ) );
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect,
                                                 s_example_sta_netif ) );
#if CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &example_handler_on_sta_got_ipv6, NULL));
#endif
    
    // OWN
    ESP_LOGI( TAG, "Connecting to %s on channel %d...", wifi_config.sta.ssid, wifi_config.sta.channel );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifi_config ) );
    esp_err_t ret = esp_wifi_connect();
    if ( ret != ESP_OK ) {
        ESP_LOGE( TAG, "WiFi connect failed! ret:%x", ret );
        return ret;
    }
    if ( wait ) {
        ESP_LOGI( TAG, "Waiting for IP(s)" );
#if CONFIG_EXAMPLE_CONNECT_IPV4
        xSemaphoreTake( s_semph_get_ip_addrs, portMAX_DELAY );
#endif
#if CONFIG_EXAMPLE_CONNECT_IPV6
        xSemaphoreTake(s_semph_get_ip6_addrs, portMAX_DELAY);
#endif
        if ( s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY ) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t example_wifi_sta_do_disconnect( void ) {
    ESP_ERROR_CHECK( esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                                   &example_handler_on_wifi_disconnect ) );
    ESP_ERROR_CHECK( esp_event_handler_unregister( IP_EVENT, IP_EVENT_STA_GOT_IP, &example_handler_on_sta_got_ip ) );
    ESP_ERROR_CHECK(
            esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect ) );
#if CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &example_handler_on_sta_got_ipv6));
#endif
    if ( s_semph_get_ip_addrs ) {
        vSemaphoreDelete( s_semph_get_ip_addrs );
    }
#if CONFIG_EXAMPLE_CONNECT_IPV6
    if (s_semph_get_ip6_addrs) {
        vSemaphoreDelete(s_semph_get_ip6_addrs);
    }
#endif
    return esp_wifi_disconnect();
}

void example_wifi_shutdown( void ) {
    example_wifi_sta_do_disconnect();
    example_wifi_stop();
}

esp_err_t example_wifi_connect( void ) {
    ESP_LOGI( TAG, "Start example_connect." );
    example_wifi_start();
    wifi_config_t wifi_config = {
            .sta = {
#if !CONFIG_EXAMPLE_WIFI_SSID_PWD_FROM_STDIN
                    .ssid = CONFIG_EXAMPLE_WIFI_SSID,
                    .password = CONFIG_EXAMPLE_WIFI_PASSWORD,
#endif
                    .scan_method = WIFI_ALL_CHANNEL_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
    };
    // OWN start
    strncpy( (char *) wifi_config.sta.ssid, known_wifi_networks[ selected_network_index ].ssid,
             sizeof(wifi_config.sta.ssid) );
    strncpy( (char *) wifi_config.sta.password, known_wifi_networks[ selected_network_index ].password,
             sizeof(wifi_config.sta.password) );
//    wifi_config.sta.channel = selected_channel;
    // OWN end
#if CONFIG_EXAMPLE_WIFI_SSID_PWD_FROM_STDIN
    example_configure_stdin_stdout();
    char buf[sizeof(wifi_config.sta.ssid)+sizeof(wifi_config.sta.password)+2] = {0};
    ESP_LOGI(TAG, "Please input ssid password:");
    fgets(buf, sizeof(buf), stdin);
    int len = strlen(buf);
    buf[len-1] = '\0'; /* removes '\n' */
    memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));

    char *rest = NULL;
    char *temp = strtok_r(buf, " ", &rest);
    strncpy((char*)wifi_config.sta.ssid, temp, sizeof(wifi_config.sta.ssid));
    memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
    temp = strtok_r(NULL, " ", &rest);
    if (temp) {
        strncpy((char*)wifi_config.sta.password, temp, sizeof(wifi_config.sta.password));
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
#endif
    // OWN changed
    return example_wifi_sta_do_connect( wifi_config, !ASYNC_WIFI_INIT );
}

// see esp-idf/examples/wifi/scan/main/scan.c

static void wifi_scan( void ) {
    ESP_ERROR_CHECK( esp_netif_init() );
//    esp_netif_t *netif = esp_netif_get_default_netif();
    // OWN commented out
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert( sta_netif );
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

#if ASYNC_WIFI_INIT
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &handler_on_wifi_scan_done,
                                                 sta_netif ) );
    esp_wifi_scan_start( NULL, false );
#else
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    uint16_t mem_size = DEFAULT_SCAN_LIST_SIZE * sizeof( wifi_ap_record_t );
    wifi_ap_record_t *ap_info = malloc( mem_size );
    uint16_t ap_count = 0;
    memset( ap_info, 0, mem_size );

    esp_wifi_scan_start(NULL, true );
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &number, ap_info ));
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &ap_count ));
    ESP_LOGI( TAG, "Total APs scanned = %u", ap_count );
    for ( int i = 0; ( i < DEFAULT_SCAN_LIST_SIZE ) && ( i < ap_count ); i++ ) {
        // OWN
        ESP_LOGI( TAG, "SSID %-16s channel %2d signal %3ddB", ap_info[ i ].ssid, ap_info[ i ].primary,
                  ap_info[ i ].rssi );
//        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
//        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
//        print_auth_mode(ap_info[i].authmode);
//        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
//            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
//        }
//        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
    find_known_wifi( ap_info, ap_count );
    free( ap_info );

    esp_wifi_stop();
    esp_netif_destroy_default_wifi( sta_netif );
#endif
}

// OWN start

#define DEFAULT_SCAN_LIST_SIZE 16

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
                selected_channel = ap_info[ ap ].primary;
            }
        }
    }
}

void wifi_shutdown( void ) {
    example_wifi_shutdown();
}

static void handler_on_wifi_scan_done( void *sta_netif, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data ) {
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

static void set_hostname() {
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
    ESP_LOGI( TAG, "hostname is %s", hostname );
    ESP_ERROR_CHECK( esp_netif_set_hostname( s_example_sta_netif, hostname ) );
    free( hostname );
}
// OWN end

#endif /* CONFIG_EXAMPLE_CONNECT_WIFI */
