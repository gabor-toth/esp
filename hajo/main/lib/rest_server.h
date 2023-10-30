#ifndef LIB_REST_SERVER_H
#define LIB_REST_SERVER_H

#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REST_SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[REST_SCRATCH_BUFSIZE];
} rest_server_context_t;

typedef esp_err_t (*rest_register_handlers_t)( httpd_handle_t, rest_server_context_t * );

extern esp_err_t rest_server_start( rest_register_handlers_t rest_register_handlers );

extern esp_err_t rest_register_static_files_handler( httpd_handle_t server, rest_server_context_t *rest_context,
                                                     const char *static_files_base_path );

/**
 * Receive JSON body.
 * Caller is responsible to call cJSON_Delete(root) and httpd_resp_sendstr()
 */
extern esp_err_t rest_receive_json_body( httpd_req_t *req, rest_server_context_t *context, cJSON **root );

#ifdef __cplusplus
}
#endif

#endif //LIB_REST_SERVER_H
