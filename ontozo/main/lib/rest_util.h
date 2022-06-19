#ifndef ONTOZO_REST_UTIL_H
#define ONTOZO_REST_UTIL_H

#include <esp_http_server.h>
#include "cJSON.h"

extern void rest_set_json_content_type( httpd_req_t *req );

extern esp_err_t rest_set_error_code( httpd_req_t *req, esp_err_t esp_err, const char *message );

extern esp_err_t rest_parse_index( const char *uri, int *index, bool needed );

extern void rest_send_message_back( httpd_req_t *req, const char *format, ... );

/* Will free root */
extern void rest_send_json_back( httpd_req_t *req, cJSON *root );

extern void rest_add_time_json( cJSON *root );

#endif //ONTOZO_REST_UTIL_H
