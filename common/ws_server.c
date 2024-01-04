/* Simple HTTP + SSL + WS Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "rest_server.h"
#include "ws_keep_alive.h"
#include "ws_server.h"
#include "sdkconfig.h"

#if !CONFIG_HTTPD_WS_SUPPORT
#error This example cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    char *message;
};

static const char *TAG = "ws_server";
#define MAX_CLIENTS 16

static esp_err_t ws_handler( httpd_req_t *req ) {
    if ( req->method == HTTP_GET ) {
        ESP_LOGI( TAG, "Handshake done, the new connection was opened" );
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset( &ws_pkt, 0, sizeof( httpd_ws_frame_t ));

    // First receive the full ws message
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame( req, &ws_pkt, 0 );
    if ( ret != ESP_OK ) {
        ESP_LOGE( TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret );
        return ret;
    }
    ESP_LOGI( TAG, "frame len is %d", ws_pkt.len );
    if ( ws_pkt.len ) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc( 1, ws_pkt.len + 1 );
        if ( buf == NULL) {
            ESP_LOGE( TAG, "Failed to calloc memory for buf" );
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame( req, &ws_pkt, ws_pkt.len );
        if ( ret != ESP_OK ) {
            ESP_LOGE( TAG, "httpd_ws_recv_frame failed with %d", ret );
            free( buf );
            return ret;
        }
    }
    // If it was a PONG, update the keep-alive
    if ( ws_pkt.type == HTTPD_WS_TYPE_PONG ) {
        ESP_LOGI( TAG, "Received PONG message" );
        free( buf );
        return wss_keep_alive_client_is_active( wss_keep_alive_get_keep_alive( req->handle ),
                                                httpd_req_to_sockfd( req ));

        // If it was a TEXT message, just echo it back
    } else if ( ws_pkt.type == HTTPD_WS_TYPE_TEXT || ws_pkt.type == HTTPD_WS_TYPE_PING ||
                ws_pkt.type == HTTPD_WS_TYPE_CLOSE ) {
        if ( ws_pkt.type == HTTPD_WS_TYPE_TEXT ) {
            ESP_LOGI( TAG, "Received packet with message: %s", ws_pkt.payload );
        } else if ( ws_pkt.type == HTTPD_WS_TYPE_PING ) {
            // Response PONG packet to peer
            ESP_LOGI( TAG, "Got a WS PING frame, Replying PONG" );
            ws_pkt.type = HTTPD_WS_TYPE_PONG;
        } else if ( ws_pkt.type == HTTPD_WS_TYPE_CLOSE ) {
            // Response CLOSE packet with no payload to peer
            ESP_LOGI( TAG, "Closed connection %d", httpd_req_to_sockfd( req ));
            ws_pkt.len = 0;
            ws_pkt.payload = NULL;
        }
        ret = httpd_ws_send_frame( req, &ws_pkt );
        if ( ret != ESP_OK ) {
            ESP_LOGE( TAG, "httpd_ws_send_frame failed with %d", ret );
        }
//        ESP_LOGI( TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
//                  httpd_req_to_sockfd( req ), httpd_ws_get_fd_info( req->handle, httpd_req_to_sockfd( req )));
        free( buf );
        return ret;
    }
    free( buf );
    return ESP_OK;
}

esp_err_t wss_open_fd( httpd_handle_t hd, int sockfd ) {
    ESP_LOGI( TAG, "New client connected %d", sockfd );
    wss_keep_alive_t h = wss_keep_alive_get_keep_alive( hd );
    return wss_keep_alive_add_client( h, sockfd );
}

void wss_close_fd( httpd_handle_t hd, int sockfd ) {
    httpd_ws_client_info_t info = httpd_ws_get_fd_info( hd, sockfd );
    ESP_LOGI( TAG, "Client disconnected %d type %d", sockfd, info );
    wss_keep_alive_t h = wss_keep_alive_get_keep_alive( hd );
    wss_keep_alive_remove_client( h, sockfd );
    close( sockfd );
}

static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true,
};


static void send_hello( void *arg ) {
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset( &ws_pkt, 0, sizeof( httpd_ws_frame_t ));
    ws_pkt.payload = (uint8_t *) resp_arg->message;
    ws_pkt.len = strlen( resp_arg->message );
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async( hd, fd, &ws_pkt );
    free( resp_arg->message );
    free( resp_arg );
}

static void send_ping( void *arg ) {
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset( &ws_pkt, 0, sizeof( httpd_ws_frame_t ));
    ws_pkt.payload = NULL;
    ws_pkt.len = 0;
    ws_pkt.type = HTTPD_WS_TYPE_PING;

    httpd_ws_send_frame_async( hd, fd, &ws_pkt );
    free( resp_arg );
}

bool client_not_alive_cb( wss_keep_alive_t h, int fd ) {
    ESP_LOGI( TAG, "Client not alive, closing fd %d", fd );
    httpd_sess_trigger_close( wss_keep_alive_get_http_server( h ), fd );
    return true;
}

bool check_client_alive_cb( wss_keep_alive_t h, int fd ) {
    ESP_LOGD( TAG, "Checking if client (fd=%d) is alive", fd );
    struct async_resp_arg *resp_arg = malloc( sizeof( struct async_resp_arg ));
    resp_arg->hd = wss_keep_alive_get_http_server( h );
    resp_arg->fd = fd;

    if ( httpd_queue_work( resp_arg->hd, send_ping, resp_arg ) == ESP_OK ) {
        return true;
    }
    return false;
}

static void start_wss_echo_server( httpd_handle_t hd ) {
    // Prepare keep-alive engine
    wss_keep_alive_config_t keep_alive_config = KEEP_ALIVE_CONFIG_DEFAULT();
    keep_alive_config.max_clients = MAX_CLIENTS;
    keep_alive_config.client_not_alive_cb = client_not_alive_cb;
    keep_alive_config.check_client_alive_cb = check_client_alive_cb;
    wss_keep_alive_start( &keep_alive_config, hd );
//    conf.open_fn = wss_open_fd;
//    conf.close_fn = wss_close_fd;
}

static void stop_wss_echo_server( httpd_handle_t server ) {
    // Stop keep alive thread
    wss_keep_alive_stop( wss_keep_alive_get_keep_alive( server ));
}

esp_err_t wss_wifi_connect( httpd_handle_t hd, const char *wifi_ssid ) {
    (void) wifi_ssid;

    start_wss_echo_server( hd );
    return rest_register_uri_handler( hd, TAG, &ws );
}

static const rest_callbacks_t callbacks = {
        .name= "ws_server",
        .wifi_connect_fn = wss_wifi_connect,
        .wifi_disconnect_fn= stop_wss_echo_server,
        .open_fn=wss_open_fd,
        .close_fn = wss_close_fd
};

void wss_register() {
    ESP_LOGI( TAG, "wss_register" );
    rest_register_callbacks( &callbacks );
}

// Get all clients and send async message
void wss_server_send_message( httpd_handle_t server, const char *message ) {
    size_t clients = MAX_CLIENTS;
    int client_fds[MAX_CLIENTS];
    if ( httpd_get_client_list( server, &clients, client_fds ) != ESP_OK ) {
        ESP_LOGE( TAG, "httpd_get_client_list failed!" );
        return;
    }
    for ( size_t i = 0; i < clients; ++i ) {
        int sock = client_fds[ i ];
        if ( httpd_ws_get_fd_info( server, sock ) != HTTPD_WS_CLIENT_WEBSOCKET ) {
            continue;
        }
        ESP_LOGI( TAG, "Active client (fd=%d) -> sending async message", sock );
        struct async_resp_arg *resp_arg = malloc( sizeof( struct async_resp_arg ));
        resp_arg->hd = server;
        resp_arg->fd = sock;
        resp_arg->message = strdup( message );
        if ( httpd_queue_work( resp_arg->hd, send_hello, resp_arg ) != ESP_OK ) {
            ESP_LOGE( TAG, "httpd_queue_work failed!" );
            break;
        }
    }
}
