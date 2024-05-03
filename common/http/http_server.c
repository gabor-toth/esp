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
#include "http_server.h"

static const char *TAG = "http-server";
static httpd_handle_t http_server = NULL;

typedef struct http_callbacks_node_t {
    struct http_callbacks_node_t *next;
    http_callbacks_t callbacks;
} http_callbacks_node_t;

static http_callbacks_node_t *registered_callbacks = NULL;
static http_callbacks_node_t *registered_callbacks_tail = NULL;
static char *wifi_ssid = NULL;

static esp_err_t open_fn_callback( httpd_handle_t hd, int sockfd ) {
    for (http_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.open_fn ) {
            esp_err_t result = node->callbacks.open_fn( hd, sockfd );
            if ( result != ESP_OK ) {
                return result;
            }
        }
    }
    return ESP_OK;
}

static void close_fn_callback( httpd_handle_t hd, int sockfd ) {
    for (http_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.close_fn ) {
            node->callbacks.close_fn( hd, sockfd );
        }
    }
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
    for (http_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.wifi_connect_fn ) {
            ESP_LOGI( TAG, "Callback for %s", node->callbacks.name );
            node->callbacks.wifi_connect_fn( http_server, wifi_ssid );
        } else {
            ESP_LOGI( TAG, "No callback for %s", node->callbacks.name );
        }
    }
    ESP_LOGI( TAG, "Done callbacks" );

    return result;
}

static esp_err_t http_server_stop() {
    for (http_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.wifi_disconnect_fn ) {
            node->callbacks.wifi_disconnect_fn( http_server );
        }
    }
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

esp_err_t http_register_callbacks(const http_callbacks_t *callbacks ) {
    http_callbacks_node_t *node = malloc(sizeof( http_callbacks_node_t ));
    if ( node == NULL) {
        return ESP_ERR_NO_MEM;
    }
    node->callbacks = *callbacks;
    node->next = NULL;
    if ( registered_callbacks_tail == NULL) {
        registered_callbacks_tail = registered_callbacks = node;
    } else {
        registered_callbacks_tail->next = node;
        registered_callbacks_tail = node;
    }
    return ESP_OK;
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