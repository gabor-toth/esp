#ifndef LIB_REST_SERVER_H
#define LIB_REST_SERVER_H

#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_http_server.h>

#define REST_SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[REST_SCRATCH_BUFSIZE];
} rest_server_context_t;

extern const char *REST_TAG;

extern esp_err_t rest_server_start(
        const char *static_files_base_path,
        void (*rest_register_handlers)( httpd_handle_t, rest_server_context_t * ));

#endif //LIB_REST_SERVER_H
