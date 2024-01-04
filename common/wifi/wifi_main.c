//
// Created by tothg on 2024.01.04..
//

#include "wifi_main.h"
#include "esp_log.h"
#include "logger.h"
#include "simple_list.h"

static const char *TAG = "wifi-main";

typedef struct rest_callbacks_node_t {
    struct rest_callbacks_node_t *next;
    wifi_callbacks_t callbacks;
} rest_callbacks_node_t;

static rest_callbacks_node_t *registered_callbacks = NULL;
static char *wifi_ssid = NULL;

esp_err_t wifi_register_callbacks( const wifi_callbacks_t *callbacks ) {
    rest_callbacks_node_t *node = malloc( sizeof( rest_callbacks_node_t ));
    if ( node == NULL) {
        return ESP_ERR_NO_MEM;
    }
    node->callbacks = *callbacks;
    simple_list_add_tail( &registered_callbacks, node );
    return ESP_OK;
}

static void handler_on_wifi_connect( void *dummy, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data ) {
    if ( event_id == IP_EVENT_STA_GOT_IP ) {
        ESP_LOGI( TAG, "[%s] Calling callbacks", currentTaskName());
        for ( rest_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
            if ( node->callbacks.wifi_connect_fn ) {
                ESP_LOGI( TAG, "Callback for %s", node->callbacks.name );
                node->callbacks.wifi_connect_fn( /*http_server,*/ wifi_ssid );
            } else {
                ESP_LOGI( TAG, "No callback for %s", node->callbacks.name );
            }
        }
        ESP_LOGI( TAG, "Done callbacks" );
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
    for ( rest_callbacks_node_t *node = registered_callbacks; node != NULL; node = node->next ) {
        if ( node->callbacks.wifi_disconnect_fn ) {
            node->callbacks.wifi_disconnect_fn( /*http_server */);
        }
    }
}

const char *wifi_get_ssid() {
    return wifi_ssid;
}

esp_err_t wifi_main() {
    ESP_ERROR_CHECK(
            esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_wifi_connect, NULL ));
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect, NULL ));
    ESP_ERROR_CHECK(
            esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect, NULL ));

    return ESP_OK;
}
