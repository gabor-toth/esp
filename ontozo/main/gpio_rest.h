#ifndef ONTOZO_GPIO_REST_H
#define ONTOZO_GPIO_REST_H

#include "lib/rest_server.h"

extern void rest_register_gpio_handlers( httpd_handle_t server, rest_server_context_t *rest_context );

#endif //ONTOZO_GPIO_REST_H
