#ifndef LIB_REST_SERVER_H
#define LIB_REST_SERVER_H

#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <cJSON.h>

#define REST_SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[REST_SCRATCH_BUFSIZE];
} rest_server_context_t;

extern esp_err_t rest_server_start(
        const char *static_files_base_path,
        void (*rest_register_handlers)( httpd_handle_t, rest_server_context_t * ));

/**
 * Receive JSON body.
 * Caller is responsible to call cJSON_Delete(root) and httpd_resp_sendstr()
 */
extern esp_err_t rest_receive_json_body( httpd_req_t *req, rest_server_context_t *context, cJSON **root );

extern void rest_set_json_content_type( httpd_req_t *req );

extern esp_err_t rest_set_error_code( httpd_req_t *req, esp_err_t esp_err, const char *message );

#endif //LIB_REST_SERVER_H
