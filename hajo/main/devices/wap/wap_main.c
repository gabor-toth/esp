/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <lwip/ip4_addr.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "wap_main.h"
#include "nvs_main.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

If you'd rather not, just change the below entries to strings with
the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#ifdef CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#else
#define EXAMPLE_ESP_WIFI_SSID      ""
#endif
#ifdef EXAMPLE_ESP_WIFI_PASS
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#else
#define EXAMPLE_ESP_WIFI_PASS      ""
#endif
#ifdef EXAMPLE_ESP_WIFI_CHANNEL
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#else
#define EXAMPLE_ESP_WIFI_CHANNEL      1
#endif
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN
#define WPS_MODE WPS_TYPE_PBC

#ifndef PIN2STR
#define PIN2STR( a ) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7]
#define PINSTR "%c%c%c%c%c%c%c%c"
#endif

static const char *TAG = "wifi_ap";

static void wifi_event_handler( void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data ) {
    switch ( event_id ) {
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI( TAG, "station "MACSTR" join, AID=%d",
                      MAC2STR( event->mac ), event->aid );
        }
            break;
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI( TAG, "station "MACSTR" leave, AID=%d",
                      MAC2STR( event->mac ), event->aid );
        }
            break;
        case WIFI_EVENT_AP_WPS_RG_SUCCESS: {
            ESP_LOGI( TAG, "WIFI_EVENT_AP_WPS_RG_SUCCESS" );
            wifi_event_ap_wps_rg_success_t *evt = (wifi_event_ap_wps_rg_success_t *) event_data;
            ESP_LOGI( TAG, "station "MACSTR" WPS successful",
                      MAC2STR( evt->peer_macaddr ) );
        }
            break;
        case WIFI_EVENT_AP_WPS_RG_FAILED: {
            ESP_LOGI( TAG, "WIFI_EVENT_AP_WPS_RG_FAILED" );
            wifi_event_ap_wps_rg_fail_reason_t *evt = (wifi_event_ap_wps_rg_fail_reason_t *) event_data;
            ESP_LOGI( TAG, "station "MACSTR" WPS failed, reason=%d",
                      MAC2STR( evt->peer_macaddr ), evt->reason );
            ESP_ERROR_CHECK( esp_wifi_ap_wps_disable() );
//            ESP_ERROR_CHECK(esp_wifi_ap_wps_enable(&config));
//            ESP_ERROR_CHECK(esp_wifi_ap_wps_start(pin));
        }
            break;
        case WIFI_EVENT_AP_WPS_RG_TIMEOUT: {
            ESP_LOGI( TAG, "WIFI_EVENT_AP_WPS_RG_TIMEOUT" );
            ESP_ERROR_CHECK( esp_wifi_ap_wps_disable() );
//            ESP_ERROR_CHECK(esp_wifi_ap_wps_enable(&config));
//            ESP_ERROR_CHECK(esp_wifi_ap_wps_start(pin));
        }
            break;
        case WIFI_EVENT_AP_WPS_RG_PIN: {
            ESP_LOGI( TAG, "WIFI_EVENT_AP_WPS_RG_PIN" );
            /* display the PIN code */
            wifi_event_ap_wps_rg_pin_t *event = (wifi_event_ap_wps_rg_pin_t *) event_data;
            ESP_LOGI( TAG, "WPS_PIN = " PINSTR, PIN2STR( event->pin_code ) );
        }
            break;
    }
}

void wifi_init_softap( void ) {
    char ssid[sizeof(((wifi_sta_config_t *) 0)->ssid)];
    char password[sizeof(((wifi_sta_config_t *) 0)->password)];
    
    ESP_ERROR_CHECK( esp_netif_init() );
    esp_netif_t *wifiAP = esp_netif_create_default_wifi_ap();
    
    // see https://www.esp32.com/viewtopic.php?t=13371
    esp_netif_ip_info_t ipInfo;
    IP4_ADDR( &ipInfo.ip, 192, 168, 77, 1 );
    IP4_ADDR( &ipInfo.gw, 192, 168, 77, 1 );
    IP4_ADDR( &ipInfo.netmask, 255, 255, 255, 0 );
    esp_netif_dhcps_stop( wifiAP );
    esp_netif_set_ip_info( wifiAP, &ipInfo );
    esp_netif_dhcps_start( wifiAP );
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    
    ESP_ERROR_CHECK( esp_event_handler_instance_register( WIFI_EVENT,
                                                          ESP_EVENT_ANY_ID,
                                                          &wifi_event_handler,
                                                          NULL,
                                                          NULL ) );
    uint8_t chipid[6];
    esp_efuse_mac_get_default( chipid );
    if ( strlen( EXAMPLE_ESP_WIFI_SSID ) != 0 ) {
        strncpy( ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(ssid) );
    } else {
        snprintf( ssid, sizeof(ssid), "wifi-%02x%02x%02x%02x%02x%02x",
                  chipid[ 0 ], chipid[ 1 ], chipid[ 2 ], chipid[ 3 ], chipid[ 4 ], chipid[ 5 ] );
    }
    if ( strlen( EXAMPLE_ESP_WIFI_PASS ) != 0 ) {
        strncpy( password, EXAMPLE_ESP_WIFI_PASS, sizeof(password) );
    } else {
        snprintf( password, sizeof(password), "pass-%02x%02x%02x%02x%02x%02x",
                  chipid[ 0 ], chipid[ 1 ], chipid[ 2 ], chipid[ 3 ], chipid[ 4 ], chipid[ 5 ] );
    }
    size_t ssid_len = strlen( ssid );
    wifi_config_t wifi_config = {
            .ap = {
                    .ssid_len = ssid_len,
                    .channel = EXAMPLE_ESP_WIFI_CHANNEL,
                    .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
                    //.authmode = WIFI_AUTH_WPA3_PSK,
                    .authmode = WIFI_AUTH_WPA2_PSK,
//                    .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
                    .authmode = WIFI_AUTH_WPA2_PSK,
#endif
                    .pmf_cfg = {
//                            .required = true,
                    },
            },
    };
    strncpy( (char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) );
    strncpy( (char *) wifi_config.ap.password, password, sizeof(wifi_config.ap.password) );
    if ( strlen( password ) == 0 ) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_AP ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_AP, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    
    ESP_LOGI( TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
              ssid, password, EXAMPLE_ESP_WIFI_CHANNEL );
}

void wap_main( void ) {
    // NVS should be initialized
    wifi_init_softap();
    wap_start_wps();
}

void wap_start_wps() {
    static esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT( WPS_MODE );
    
    ESP_ERROR_CHECK( esp_wifi_ap_wps_enable( &config ) );
#if EXAMPLE_WPS_PIN_VALUE
    ESP_LOGI(TAG, "Staring WPS registrar with user specified pin %s", pin);
    snprintf((char *)pin, 9, "%08d", EXAMPLE_WPS_PIN_VALUE);
    ESP_ERROR_CHECK(esp_wifi_ap_wps_start(pin));
#else
    if ( config.wps_type == WPS_TYPE_PBC ) {
        ESP_LOGI( TAG, "Starting WPS registrar in PBC mode" );
    } else {
        ESP_LOGI( TAG, "Starting WPS registrar with random generated pin" );
    }
    ESP_ERROR_CHECK( esp_wifi_ap_wps_start( NULL ) );
#endif
    
    // esp_wifi_ap_wps_disable
    
    // E (15688) wpa: WPS: fail event msg=12 config_error=13 error_indication=0
}