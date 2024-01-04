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
#include "lwip/apps/mdns.h"
#include "lwip/apps/netbiosns.h"
#include "rest_main.h"
#include "rest_server.h"

static const char *TAG = "rest-main";

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

static err_enum_t on_wifi_connect( httpd_handle_t server, const char *wifi_ssid ) {
    (void) server;
    (void) wifi_ssid;

//    initialise_netbios();
    return ESP_OK;
}

static void on_wifi_disconnect( httpd_handle_t server ) {
    (void) server;

//    netbiosns_stop();
}

static const rest_callbacks_t callbacks = {
        .name= "rest-main",
        .wifi_connect_fn = on_wifi_connect,
        .wifi_disconnect_fn= on_wifi_disconnect,
        .open_fn=NULL,
        .close_fn = NULL
};

void discovery_register() {
    wifi_register_callbacks( &callbacks );
}

#endif
