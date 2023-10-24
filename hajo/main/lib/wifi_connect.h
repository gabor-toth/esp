#ifndef ONTOZO_WIFI_H
#define ONTOZO_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

extern esp_err_t wifi_connect();

extern void wifi_shutdown( void );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_WIFI_H
