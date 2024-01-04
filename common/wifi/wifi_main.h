#ifndef ONTOZO_WIFI_H
#define ONTOZO_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <esp_http_server.h>
#include "esp_wifi.h"

typedef esp_err_t (*wifi_connect_func_t)( httpd_handle_t server, const char *wifi_ssid );

typedef void (*wifi_disconnect_func_t)( httpd_handle_t server );

//typedef esp_err_t (*register_handlers_t)( httpd_handle_t server, void *dummy );

typedef struct wifi_callbacks_t {
    struct wifi_callbacks_t *next;
    const char *name;
    wifi_connect_func_t wifi_connect_fn;
    wifi_disconnect_func_t wifi_disconnect_fn;
    httpd_open_func_t open_fn;
    httpd_close_func_t close_fn;
} wifi_callbacks_t;

extern esp_err_t wifi_register_callbacks( const wifi_callbacks_t *callbacks );

extern esp_err_t wifi_connect();

extern const char *wifi_get_ssid();

#ifdef _ESP_NETIF_TYPES_H_

extern esp_netif_t *wifi_get_esp_netif();

#endif

extern void wifi_shutdown( void );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_WIFI_H
