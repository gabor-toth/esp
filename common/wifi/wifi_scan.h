//
// Created by apa on 2024.01.07..
//

#ifndef SENSOR_TEMP_WIFI_SCAN_H
#define SENSOR_TEMP_WIFI_SCAN_H

#include "esp_wifi_types.h"

extern void wifi_set_credentials( wifi_config_t *wifi_config );

extern void wifi_set_hostname();

extern bool wifi_wait_on_connect();

#endif //SENSOR_TEMP_WIFI_SCAN_H
