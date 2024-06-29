#ifndef ONTOZO_GPIO_REST_H
#define ONTOZO_GPIO_REST_H

#include "http/http_server.h"

extern void rest_register_gpio_handlers( httpd_handle_t server, http_server_context_t *server_context );

#endif //ONTOZO_GPIO_REST_H
