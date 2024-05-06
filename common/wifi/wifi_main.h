#ifndef HAJO_WIFI_MAIN_H
#define HAJO_WIFI_MAIN_H

#include "esp_netif_types.h"
#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_WIFI_INIT 1

ESP_EVENT_DECLARE_BASE( WIFI_OWN_EVENT );

typedef enum {
    WIFI_OWN_EVENT_RESCAN_TIMER,
    WIFI_OWN_EVENT_CONNECTION_MAX_RETRY,
} wifi_own_event_t;

extern void wifi_scan_get_ssid_and_password( wifi_sta_config_t *wifi_config_sta );

extern void wifi_set_hostname( esp_netif_t *netif );

extern void wifi_scan_start( void );

extern void wifi_handler_on_scan_done( void *sta_netif, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data );

extern esp_err_t wifi_main();

extern void wifi_shutdown( void );

extern esp_netif_t *wifi_get_esp_netif();

#ifdef __cplusplus
}
#endif

#endif //HAJO_WIFI_MAIN_H
