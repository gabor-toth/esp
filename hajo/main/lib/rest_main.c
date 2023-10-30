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
#include "lib/rest_main.h"
#include "lib/rest_server.h"
#include "wifi_connect.h"

static const char *TAG = "rest-main";

static void initialise_mdns( void ) {
    ESP_ERROR_CHECK( mdns_init());
    ESP_ERROR_CHECK( mdns_hostname_set( CONFIG_EXAMPLE_MDNS_HOST_NAME ));
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

esp_err_t init_fs( void ) {
    esp_vfs_spiffs_conf_t conf = {
            .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register( &conf );

    if ( ret != ESP_OK ) {
        if ( ret == ESP_FAIL ) {
            ESP_LOGE( TAG, "Failed to mount or format filesystem" );
        } else if ( ret == ESP_ERR_NOT_FOUND ) {
            ESP_LOGE( TAG, "Failed to find SPIFFS partition" );
        } else {
            ESP_LOGE( TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name( ret ));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used );
    if ( ret != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name( ret ));
    } else {
        ESP_LOGI( TAG, "Partition size: total: %d, used: %d", total, used );
    }
    return ESP_OK;
}

static void handler_on_wifi_connect( void *dummy, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data ) {
    ESP_ERROR_CHECK(
            esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_START, &handler_on_wifi_connect ));

    rest_init_after_wifi(NULL);
}

void rest_init_before_wifi( void ) {
    initialise_mdns();
    initialise_netbios();

    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_START, &handler_on_wifi_connect, NULL ));
}

void rest_init_after_wifi( rest_register_handlers_t rest_register_handlers ) {
    esp_netif_t *netif = wifi_get_esp_netif();
    if ( netif != NULL) {
        const char *hostname;
        ESP_ERROR_CHECK( esp_netif_get_hostname( netif, &hostname ));
        ESP_ERROR_CHECK( mdns_hostname_set( hostname ));
    }

//    ESP_ERROR_CHECK( init_fs());
//    ESP_ERROR_CHECK( rest_server_start( CONFIG_EXAMPLE_WEB_MOUNT_POINT, rest_register_handlers ));
}

#endif