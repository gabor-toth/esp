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
#include "lwip/apps/mdns.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "http_main.h"
#include "http_server.h"
#include "wifi_connect.h"

static const char *TAG = "rest-main";

static void initialise_mdns( void ) {
    ESP_ERROR_CHECK( mdns_init());
    esp_netif_t *netif = wifi_get_esp_netif();
    if ( netif != NULL) {
        const char *hostname;
        ESP_ERROR_CHECK( esp_netif_get_hostname( netif, &hostname ));
        ESP_ERROR_CHECK( mdns_hostname_set( hostname ));
    } else {
        ESP_ERROR_CHECK( mdns_hostname_set( CONFIG_EXAMPLE_MDNS_HOST_NAME ));
    }
    ESP_ERROR_CHECK( mdns_instance_name_set( CONFIG_MDNS_INSTANCE_NAME ));

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
            sizeof( serviceTxtData ) / sizeof( serviceTxtData[ 0 ] )));
    /*
    ESP_ERROR_CHECK( mdns_service_subtype_add_for_host(
            CONFIG_MDNS_INSTANCE_NAME,
            "_http",
            "_tcp",
            NULL,
            "_server" ));
    */
}

static void initialise_netbios( void ) {
    netbiosns_init();
    netbiosns_set_name( CONFIG_EXAMPLE_MDNS_HOST_NAME );
}

static esp_err_t on_wifi_connect( httpd_handle_t server, const char * wifi_ssid ) {
    initialise_mdns();
//    initialise_netbios();
    return ESP_OK;
}

static void on_wifi_disconnect( httpd_handle_t server ) {
    mdns_free();
//    netbiosns_stop();
}

static const http_callbacks_t callbacks = {
        .name= "rest-main",
        .wifi_connect_fn = on_wifi_connect,
        .wifi_disconnect_fn= on_wifi_disconnect,
        .open_fn=NULL,
        .close_fn = NULL
};

void discovery_register() {
    http_register_callbacks(&callbacks);
}

#endif
