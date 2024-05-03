#ifndef ONTOZO_SYSTEM_INFO_REST_H
#define ONTOZO_SYSTEM_INFO_REST_H

#ifdef __cplusplus
extern "C" {
#endif

extern void rest_register_system_info_handler(httpd_handle_t server, http_server_context_t *rest_context );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_SYSTEM_INFO_REST_H
