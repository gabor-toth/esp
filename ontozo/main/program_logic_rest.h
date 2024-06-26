#ifndef ONTOZO_PROGRAM_LOGIC_REST_H
#define ONTOZO_PROGRAM_LOGIC_REST_H

#include "http/http_server.h"

extern void rest_register_program_logic_handlers( httpd_handle_t server, http_server_context_t *rest_context );

#endif //ONTOZO_PROGRAM_LOGIC_REST_H
