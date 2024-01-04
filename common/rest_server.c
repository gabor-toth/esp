#include "sdkconfig.h"
#include "logger.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)
/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <cJSON.h>
#include <errno.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <fcntl.h>
#include "rest_server.h"
#include "rest_util.h"
#include "simple_list.h"
#include <string.h>
#include "wifi/wifi_main.h"

static const char *TAG = "rest-server";
static httpd_handle_t http_server = NULL;

typedef struct rest_callbacks_node_t {
    struct rest_callbacks_node_t *next;
    rest_callbacks_t callbacks;
} rest_callbacks_node_t;

static rest_callbacks_node_t *registered_callbacks = NULL;

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)

#define CHECK_FILE_EXTENSION( filename, ext ) (strcasecmp(&(filename)[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
esp_err_t set_content_type_from_file( httpd_req_t *req, const char *filepath ) {
    const char *type = "text/plain";
    if ( CHECK_FILE_EXTENSION( filepath, ".html" ) ) {
        type = HTTPD_TYPE_TEXT;
    } else if ( CHECK_FILE_EXTENSION( filepath, ".js" ) ) {
        type = "application/javascript";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".css" ) ) {
        type = "text/css";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".png" ) ) {
        type = "image/png";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".ico" ) ) {
        type = "image/x-icon";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".svg" ) ) {
        type = "text/xml";
    }
    return httpd_resp_set_type( req, type );
}

static bool has_2_dots_in_file_name( char *filepath ) {
    char *p;

    return ((p = strchr( filepath, '.' )) != NULL) && strchr( p + 1, '.' ) != NULL;
}

static void set_cache_forever( httpd_req_t *req, char *filepath ) {
    ESP_LOGI( TAG, "Set cache forever for %s", filepath );
    /*
    Last-Modified: Mon, 08 Dec 2014 19:23:51 GMT
    ETag: "5485fac7-ae74"
    Cache-Control: max-age=533280
    Expires: Sun, 03 May 2015 23:02:37 GMT
     */
//    static char last_modified_header_value[32];
//    static char max_age_header_value[32];

    httpd_resp_set_hdr( req, "Cache-Control", "max-age=31536000" ); // 1 year in seconds

//    struct stat file_state;
//    stat( filepath, &file_state );
//    struct tm timeinfo = { 0 };
//    localtime_r( &file_state.st_mtim.tv_sec, &timeinfo );
//    strftime( last_modified_header_value, sizeof last_modified_header_value, "%r", &timeinfo );
//    httpd_resp_set_hdr( req, "Last-Modified", last_modified_header_value );
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_file_get_handler( httpd_req_t *req ) {
    char filepath[FILE_PATH_MAX];
    char error_message[255];

    rest_server_context_t *rest_context = (rest_server_context_t *) req->user_ctx;
    strlcpy( filepath, rest_context->base_path, sizeof(filepath) );
    if ( req->uri[strlen( req->uri ) - 1] == '/' || !strchr( req->uri, '.' ) ) {
        // serve index.html for Angular routes
        strlcat( filepath, "/index.html", sizeof(filepath) );
    } else {
        strlcat( filepath, req->uri, sizeof(filepath) );
    }
    int fd = open( filepath, O_RDONLY, 0 );
    if ( fd == -1 ) {
        ESP_LOGW( TAG, "Failed to open file : %s", filepath );
        snprintf( error_message, sizeof(error_message), "Failed to read file: %d", errno );
        httpd_resp_send_err( req, HTTPD_404_NOT_FOUND, error_message );
        return ESP_FAIL;
    }

    ESP_LOGI( TAG, "Sending file %s", filepath );
    if ( has_2_dots_in_file_name( filepath ) ) {
        set_cache_forever( req, filepath );
    }
    set_content_type_from_file( req, filepath );

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read( fd, chunk, REST_SCRATCH_BUFSIZE );
        if ( read_bytes == -1 ) {
            ESP_LOGE( TAG, "Failed to read file : %s", filepath );
        } else if ( read_bytes > 0 ) {
            /* Send the buffer contents as HTTP response chunk */
            if ( httpd_resp_send_chunk( req, chunk, read_bytes ) != ESP_OK ) {
                close( fd );
                ESP_LOGE( TAG, "File sending failed!" );
                /* Abort sending file */
                httpd_resp_sendstr_chunk( req, NULL );
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file" );
                return ESP_FAIL;
            }
        }
    } while ( read_bytes > 0 );
    /* Close file after sending complete */
    close( fd );
    ESP_LOGI( TAG, "File sending complete" );
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk( req, NULL, 0 );
    return ESP_OK;
}

esp_err_t rest_receive_json_body( httpd_req_t *req, rest_server_context_t *context, cJSON **root ) {
    *root = 0;

    int total_len = (int) req->content_len;
    int cur_len = 0;
    char *buf = context->scratch;
    if ( total_len >= REST_SCRATCH_BUFSIZE ) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "content too long" );
        return ESP_FAIL;
    }
    while ( cur_len < total_len ) {
        int received = httpd_req_recv( req, buf + cur_len, total_len );
        if ( received <= 0 ) {
            httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value" );
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    *root = cJSON_Parse( buf );
    return ESP_OK;
}

esp_err_t rest_register_static_files_handler( httpd_handle_t server, rest_server_context_t *rest_context,
                                              const char *static_files_base_path ) {
    if ( static_files_base_path == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }
    strlcpy( rest_context->base_path, static_files_base_path, sizeof(rest_context->base_path) );
    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = rest_file_get_handler,
            .user_ctx = rest_context
    };
    return rest_register_uri_handler( server, TAG, &common_get_uri );
}

static esp_err_t open_fn_callback( httpd_handle_t hd, int sockfd ) {
    for ( rest_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
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
    for ( rest_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.close_fn ) {
            node->callbacks.close_fn( hd, sockfd );
        }
    }
}

static esp_err_t rest_server_start( const char *wifi_ssid ) {
//    rest_server_context_t *rest_context = calloc( 1, sizeof( rest_server_context_t ));
//    if ( rest_context == NULL) {
//        ESP_LOGE( TAG, "No memory for rest_context" );
//        return ESP_ERR_NO_MEM;
//    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;
    config.global_user_ctx = calloc( GLOBAL_USER_CONTEXT_COUNT, sizeof( void * ) );
    config.open_fn = open_fn_callback;
    config.close_fn = close_fn_callback;

    ESP_LOGI( TAG, "Starting HTTP Server" );
    esp_err_t result = httpd_start( &http_server, &config );
    if ( result != ESP_OK ) {
        ESP_LOGE( TAG, "Error %d starting HTTP Server", result );
//        free( rest_context );
        return result;
    }

    ESP_LOGI( TAG, "[%s] Calling callbacks", currentTaskName() );
    for ( rest_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.start_fn ) {
            node->callbacks.start_fn( http_server, wifi_ssid );
        }
    }
    ESP_LOGI( TAG, "Done callbacks" );

    return result;
}

static esp_err_t rest_server_stop() {
    return httpd_stop( http_server );
}

esp_err_t rest_register_uri_handler( httpd_handle_t handle,
                                     const char *log_tag,
                                     const httpd_uri_t *uri_handler ) {
    ESP_LOGI( log_tag, "Register URL %s", uri_handler->uri );
    return httpd_register_uri_handler( handle, uri_handler );
}

static err_enum_t on_wifi_connect( const char *wifi_ssid ) {
    (void) wifi_ssid;

    if ( http_server == NULL ) {
        ESP_ERROR_CHECK( rest_server_start( wifi_ssid ) );
    }
//    initialise_netbios();
    return ESP_OK;
}

static void on_wifi_disconnect() {
    if ( http_server ) {
        if ( rest_server_stop() == ESP_OK ) {
            http_server = NULL;
        } else {
            ESP_LOGE( TAG, "Failed to stop HTTP server" );
        }
    }
//    netbiosns_stop();
}

esp_err_t rest_register_callbacks( const rest_callbacks_t *callbacks ) {
    rest_callbacks_node_t *node = malloc( sizeof( rest_callbacks_node_t ) );
    if ( node == NULL ) {
        return ESP_ERR_NO_MEM;
    }
    node->callbacks = *callbacks;
    simple_list_add_tail( &registered_callbacks, node );
    return ESP_OK;
}

static const wifi_callbacks_t wifi_callbacks = {
        .name= "rest-main",
        .wifi_connect_fn = on_wifi_connect,
        .wifi_disconnect_fn= on_wifi_disconnect,
};

esp_err_t rest_server_main() {
    wifi_register_callbacks( &wifi_callbacks );

    return ESP_OK;
}

#endif