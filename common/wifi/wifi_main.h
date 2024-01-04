#ifndef ONTOZO_WIFI_H
#define ONTOZO_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <esp_http_server.h>
#include "esp_wifi.h"
#include "lwip/err.h"

typedef esp_err_t (*wifi_connect_func_t)(const char *wifi_ssid );

typedef void (*wifi_disconnect_func_t)();

typedef struct wifi_callbacks_t {
    struct wifi_callbacks_t *next;
    const char *name;
    wifi_connect_func_t wifi_connect_fn;
    wifi_disconnect_func_t wifi_disconnect_fn;
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
