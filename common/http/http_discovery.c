#include "sdkconfig.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)
/* HTTP Restful API Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "http_events.h"
#include "http_discovery.h"
#include "http_server.h"
#include "wifi/wifi_main.h"

static const char *TAG = "http_discovery";

static void initialise_mdns( void ) {
    ESP_ERROR_CHECK( mdns_init() );
    esp_netif_t *netif = wifi_get_esp_netif();
    if ( netif != NULL ) {
        const char *hostname;
        ESP_ERROR_CHECK( esp_netif_get_hostname( netif, &hostname ) );
        ESP_ERROR_CHECK( mdns_hostname_set( hostname ) );
    } else {
        ESP_ERROR_CHECK( mdns_hostname_set( CONFIG_EXAMPLE_MDNS_HOST_NAME ) );
    }
    ESP_ERROR_CHECK( mdns_instance_name_set( CONFIG_MDNS_INSTANCE_NAME ) );

    mdns_txt_item_t serviceTxtData[] = {
            { "board", "esp32s2" },
            { "path",  "/" }
    };

    ESP_ERROR_CHECK( mdns_service_add(
            CONFIG_MDNS_INSTANCE_NAME,
            "_http",
            "_tcp",
            80,
            serviceTxtData,
            sizeof(serviceTxtData) / sizeof(serviceTxtData[ 0 ]) ) );
    /*
    ESP_ERROR_CHECK( mdns_service_subtype_add_for_host(
            CONFIG_MDNS_INSTANCE_NAME,
            "_http",
            "_tcp",
            NULL,
            "_server" ));
    */
}

__attribute__((unused))
static void initialise_netbios( void ) {
    netbiosns_init();
    netbiosns_set_name( CONFIG_EXAMPLE_MDNS_HOST_NAME );
}

static void on_http_server_start( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
//    http_server_server_event_data * data = event_data;
    initialise_mdns();
//    initialise_netbios();
}

static void on_http_server_stop( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "on_http_server_stop start" );
    mdns_free();
//    netbiosns_stop();
    ESP_LOGI( TAG, "on_http_server_stop end" );
}

void discovery_register() {
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START,
                                        &on_http_server_start, NULL ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPING,
                                        &on_http_server_stop, NULL ) );
}

#endif
