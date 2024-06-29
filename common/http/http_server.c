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

static const char *TAG = "http_server";
static httpd_handle_t http_server = NULL;
static bool stopping = false;

ESP_EVENT_DEFINE_BASE( HTTP_SERVER_EVENT );

static char *wifi_ssid = NULL;

static esp_err_t open_fn_callback( httpd_handle_t hd, int sockfd ) {
    http_server_file_descriptor_event_data data = {
            .hd = hd,
            .sockfd = sockfd
    };
    return esp_event_post( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_FILE_DESCRIPTOR_OPEN, &data, sizeof( data ),
                           portMAX_DELAY );
}

static void close_fn_callback( httpd_handle_t hd, int sockfd ) {
    http_server_file_descriptor_event_data data = {
            .hd = hd,
            .sockfd = sockfd
    };
    ESP_ERROR_CHECK( esp_event_post( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_FILE_DESCRIPTOR_CLOSE,
                                     &data, sizeof( data ), portMAX_DELAY ) );
}

static void free_global_user_ctx( void *ctx ) {
    void **global_context = ctx;
    for ( int i = 0; i < GLOBAL_USER_CONTEXT_COUNT; i++ ) {
        if ( global_context[ i ] != NULL ) {
            free( global_context[ i ] );
        }
    }
    free( global_context );
}

static esp_err_t http_server_start( const char *wifi_ssid, size_t http_server_context_size ) {

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.close_fn = close_fn_callback;
    config.global_user_ctx = calloc( GLOBAL_USER_CONTEXT_COUNT, sizeof( void * ) );
    config.global_user_ctx_free_fn = free_global_user_ctx;
    config.max_uri_handlers = 16;
    config.open_fn = open_fn_callback;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if ( http_server_context_size > 0 ) {
        http_server_context_t *http_context = calloc( 1, http_server_context_size );
        if ( http_context == NULL ) {
            ESP_LOGE( TAG, "No memory for http_context" );
            free_global_user_ctx( config.global_user_ctx );
            return ESP_ERR_NO_MEM;
        }
        ( (void **) config.global_user_ctx )[ GLOBAL_USER_CONTEXT_SERVER_CONTEXT ] = http_context;
    }
    ESP_LOGI( TAG, "Starting HTTP Server" );
    stopping = false;
    esp_err_t result = httpd_start( &http_server, &config );
    if ( result != ESP_OK ) {
        ESP_LOGE( TAG, "Error %d starting HTTP Server", result );
        free_global_user_ctx( config.global_user_ctx );
        return result;
    }

    http_server_server_event_data data = {
            .hd = http_server,
            .ssid = wifi_ssid,
    };
    ESP_ERROR_CHECK(
            esp_event_post( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START,
                            &data, sizeof( data ), portMAX_DELAY ) );

    return result;
}

static void handler_on_wifi_connect( void *event_handler_arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data ) {
    size_t http_server_context_size = (size_t) event_handler_arg;
    if ( event_id == IP_EVENT_STA_GOT_IP ) {
        if ( http_server == NULL ) {
            ESP_ERROR_CHECK( http_server_start( wifi_ssid, http_server_context_size ) );
        }
    } else if ( event_id == WIFI_EVENT_STA_CONNECTED ) {
        wifi_event_sta_connected_t *wifi_event = event_data;
        if ( wifi_ssid != NULL ) {
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
        if ( stopping ) {
            ESP_LOGW( TAG, "Wifi disconnected, already stopping" );
            return;
        }
        stopping = true;
        http_server_server_event_data data = {
                .hd = http_server,
                .ssid = wifi_ssid,
        };
        ESP_LOGI( TAG, "Wifi disconnected, sending STOPPING event" );
        ESP_ERROR_CHECK(
                esp_event_post( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPING,
                                &data, sizeof( data ), portMAX_DELAY ) );
    }
}

static void handler_on_http_server_stopping( void *dummy, esp_event_base_t event_base,
                                             int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "Sending STOPPED event" );
    ESP_ERROR_CHECK(
            esp_event_post( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPED,
                            NULL, 0, portMAX_DELAY ) );
}

static void handler_on_http_server_stopped( void *dummy, esp_event_base_t event_base,
                                            int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "Stopping HTTP server" );
    if ( httpd_stop( http_server ) != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to stop HTTP server" );
    }
    http_server = NULL;
    ESP_LOGI( TAG, "Stopped HTTP server" );
}

esp_err_t http_register_uri_handler( httpd_handle_t handle,
                                     const char *log_tag,
                                     const httpd_uri_t *uri_handler ) {
    ESP_LOGI( log_tag, "Register URL %s", uri_handler->uri );
    return httpd_register_uri_handler( handle, uri_handler );
}

void *http_get_server_context( httpd_handle_t handle, int context_id ) {
    void **guc = httpd_get_global_user_ctx( handle );
    return guc[ context_id ];
}

esp_err_t http_server_main( size_t http_server_context_size ) {
    if ( http_server_context_size == DEFAULT_HTTP_SERVER_CONTEXT_SIZE ) {
        http_server_context_size = sizeof( http_server_context_t );
    }
    ESP_ERROR_CHECK(
            esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &handler_on_wifi_connect, (void *) http_server_context_size ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                                        &handler_on_wifi_connect, NULL ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                        &handler_on_wifi_disconnect, NULL ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPING,
                                        &handler_on_http_server_stopping, NULL ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPED,
                                        &handler_on_http_server_stopped, NULL ) );

    return ESP_OK;
}

#endif