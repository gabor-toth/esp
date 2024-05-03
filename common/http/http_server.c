#include "sdkconfig.h"
#include "logger.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)
/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <esp_log.h>
#include "esp_wifi.h"
#include <cJSON.h>
#include "http_events.h"
#include "http_server.h"

static const char *TAG = "http-server";
static httpd_handle_t http_server = NULL;

ESP_EVENT_DEFINE_BASE(HTTP_SERVER_EVENT);

static char *wifi_ssid = NULL;

static esp_err_t open_fn_callback( httpd_handle_t hd, int sockfd ) {
    http_server_file_descriptor_event_data data = {
            .hd = hd,
            .sockfd = sockfd
    };
    return esp_event_post(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_FILE_DESCRIPTOR_OPEN, &data, sizeof(data), portMAX_DELAY);
}

static void close_fn_callback( httpd_handle_t hd, int sockfd ) {
    http_server_file_descriptor_event_data data = {
            .hd = hd,
            .sockfd = sockfd
    };
    esp_event_post(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_FILE_DESCRIPTOR_CLOSE, &data, sizeof(data), portMAX_DELAY);
}

static esp_err_t http_server_start(const char *wifi_ssid ) {
//    http_server_context_t *http_context = calloc( 1, sizeof( http_server_context_t ));
//    if ( http_context == NULL) {
//        ESP_LOGE( TAG, "No memory for http_context" );
//        return ESP_ERR_NO_MEM;
//    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;
    config.global_user_ctx = calloc( GLOBAL_USER_CONTEXT_COUNT, sizeof( void * ));
    config.open_fn = open_fn_callback;
    config.close_fn = close_fn_callback;

    ESP_LOGI( TAG, "Starting HTTP Server" );
    esp_err_t result = httpd_start( &http_server, &config );
    if ( result != ESP_OK ) {
        ESP_LOGE( TAG, "Error %d starting HTTP Server", result );
//        free( http_context );
        return result;
    }

    ESP_LOGI( TAG, "[%s] Calling callbacks", currentTaskName());
    http_server_server_event_data data = {
            .hd = &http_server,
            .ssid = wifi_ssid,
    };
    esp_event_post(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START, &data, sizeof(data), portMAX_DELAY);
    ESP_LOGI( TAG, "Done callbacks" );

    return result;
}

static esp_err_t http_server_stop() {
    http_server_server_event_data data = {
            .hd = &http_server,
            .ssid = wifi_ssid,
    };
    esp_event_post(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOP, &data, sizeof(data), portMAX_DELAY);
    return httpd_stop( http_server );
}

static void handler_on_wifi_connect( void *dummy, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data ) {
    if ( event_id == IP_EVENT_STA_GOT_IP ) {
        if ( http_server == NULL) {
            ESP_ERROR_CHECK(http_server_start(wifi_ssid));
        }
    } else if ( event_id == WIFI_EVENT_STA_CONNECTED ) {
        wifi_event_sta_connected_t *wifi_event = event_data;
        if ( wifi_ssid != NULL) {
            free( wifi_ssid );
        }
        wifi_ssid = malloc( wifi_event->ssid_len + 1 );
        memcpy( wifi_ssid, wifi_event->ssid, wifi_event->ssid_len );
        wifi_ssid[ wifi_event->ssid_len ] = 0;
    }
}

static void handler_on_wifi_disconnect( void *dummy, esp_event_base_t event_base,
                                        int32_t event_id, void *event_data ) {
    if ( http_server ) {
        if (http_server_stop() == ESP_OK ) {
            http_server = NULL;
        } else {
            ESP_LOGE( TAG, "Failed to stop HTTP server" );
        }
    }
}

esp_err_t http_register_uri_handler(httpd_handle_t handle,
                                    const char *log_tag,
                                    const httpd_uri_t *uri_handler ) {
    ESP_LOGI( log_tag, "Register URL %s", uri_handler->uri );
    return httpd_register_uri_handler( handle, uri_handler );
}

esp_err_t http_server_main() {
    ESP_ERROR_CHECK(
            esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_wifi_connect, NULL ));
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect, NULL ));
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect, NULL ));

    return ESP_OK;
}

#endif