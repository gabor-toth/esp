#ifndef ONTOZO_WIFI_H
#define ONTOZO_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

extern esp_err_t wifi_connect();

#ifdef _ESP_NETIF_TYPES_H_

extern esp_netif_t *wifi_get_esp_netif();

#endif

extern void wifi_shutdown( void );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_WIFI_H
