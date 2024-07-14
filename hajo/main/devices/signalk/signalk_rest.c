#include "esp_log.h"
#include "http/http_server.h"
#include "http/http_events.h"
#include "rest_util.h"
#include "signalk_rest.h"
#include <sys/queue.h>

//#include "NMEA2000-SignalK-Gateway.h"

extern void pngSimulationStart();

extern void pngSimulationStop();

extern void pngSimulationOneOff();

static const char *TAG = "signalk_rest";

#define HTTP_URI_SIMULATE_PNGS "/simulate_pngs/"

struct list_entry_t {
    SLIST_ENTRY( list_entry_t ) entries;
    unsigned long png;
};
SLIST_HEAD( list_head_t, list_entry_t ) list_head;

static esp_err_t unhandled_pngs_get( httpd_req_t *req ) {
    cJSON *root = cJSON_CreateObject();
    cJSON *entries = cJSON_AddArrayToObject( root, "unhandled_pngs" );

    struct list_entry_t *node;
    for ( node = SLIST_FIRST( &list_head ); node != NULL; node = SLIST_NEXT( node, entries ) ) {
        cJSON_AddItemToArray( entries, cJSON_CreateNumber( node->png ) );
    }
    rest_send_json_back_and_delete( req, root );

    return ESP_OK;
}

static esp_err_t unhandled_pngs_delete( httpd_req_t *req ) {
    struct list_entry_t *node;

    while ( ( node = SLIST_FIRST( &list_head ) ) != NULL ) {
        SLIST_REMOVE( &list_head, SLIST_FIRST( &list_head ), list_entry_t, entries );
        free( node );
    }

    httpd_resp_sendstr( req, NULL );

    return ESP_OK;
}

static esp_err_t simulate_pngs( httpd_req_t *req ) {
    const char *mode = req->uri + strlen( HTTP_URI_SIMULATE_PNGS );
    if ( strcmp( mode, "start" ) == 0 ) {
        pngSimulationStart();
    } else if ( strcmp( mode, "stop" ) == 0 ) {
        pngSimulationStop();
    } else if ( strcmp( mode, "one" ) == 0 ) {
        pngSimulationOneOff();
    } else {
        return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "bad mode" );
    }
    return httpd_resp_sendstr( req, NULL );
}

static void
handler_on_http_server_start( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    http_server_server_event_data *data = event_data;

    httpd_uri_t ws = {
            .uri        = "/unhandled_pngs",
            .method     = HTTP_GET,
            .handler    = unhandled_pngs_get,
            .user_ctx   = NULL,
    };
    http_register_uri_handler( data->hd, TAG, &ws );

    ws.method = HTTP_DELETE;
    ws.handler = unhandled_pngs_delete;
    http_register_uri_handler( data->hd, TAG, &ws );

    ws.uri = HTTP_URI_SIMULATE_PNGS "*";
    ws.method = HTTP_PUT;
    ws.handler = simulate_pngs;
    http_register_uri_handler( data->hd, TAG, &ws );
}

void signalk_rest_unhandled_pgn( unsigned long png ) {
    struct list_entry_t *node;

    for ( node = SLIST_FIRST( &list_head ); node != NULL; node = SLIST_NEXT( node, entries ) ) {
        if ( node->png == png ) {
            return;
        }
    }

    node = malloc( sizeof( struct list_entry_t ) );
    node->png = png;
    SLIST_INSERT_HEAD( &list_head, node, entries );
}

void signalk_rest_register() {
    ESP_LOGI( TAG, "wss_register" );

    SLIST_INIT( &list_head );

    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START,
                                        &handler_on_http_server_start, NULL ) );
}
