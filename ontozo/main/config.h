#ifndef ONTOZO_CONFIG_H
#define ONTOZO_CONFIG_H

#include "driver/gpio.h"

#define GPIO_INPUT_LEVEL_1  GPIO_NUM_22
#define GPIO_INPUT_LEVEL_2  GPIO_NUM_21
#define GPIO_INPUT_LEVEL_3  GPIO_NUM_4
#define GPIO_INPUT_LEVEL_4  GPIO_NUM_15

#define GPIO_INPUT_BUTTON_START     GPIO_NUM_0
#define GPIO_INPUT_BUTTON_STOP      GPIO_NUM_2

#define GPIO_OUTPUT_PUMP_MAIN       GPIO_NUM_12
#define GPIO_OUTPUT_PUMP_REFILL     GPIO_NUM_27

#define GPIO_OUTPUT_ZONE_1          GPIO_NUM_23
#define GPIO_OUTPUT_ZONE_2          GPIO_NUM_19
#define GPIO_OUTPUT_ZONE_3          GPIO_NUM_18
#define GPIO_OUTPUT_ZONE_4          GPIO_NUM_5
#define GPIO_OUTPUT_ZONE_5          GPIO_NUM_32
#define GPIO_OUTPUT_ZONE_6          GPIO_NUM_33
#define GPIO_OUTPUT_ZONE_7          GPIO_NUM_25
#define GPIO_OUTPUT_ZONE_8          GPIO_NUM_13

#define CONFIG_MDNS_HOST_NAME              "ontozo"
#define CONFIG_MDNS_INSTANCE_NAME          "ontozo web server"

#endif //ONTOZO_CONFIG_H
