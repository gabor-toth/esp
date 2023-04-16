#ifndef ONTOZO_GPIO_JSON_H
#define ONTOZO_GPIO_JSON_H

#include <stdbool.h>
#include "lib/gpio_define.h"

#ifdef __cplusplus
extern "C" {
#endif

// caller should free returned pointer
extern char *gpio_data_to_json_string( PinData *config );

extern void gpio_data_from_json_string( const char *json_string, PinData *config );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_GPIO_JSON_H
