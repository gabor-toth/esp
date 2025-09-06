#ifndef LIB_REST_SERVER_H
#define LIB_REST_SERVER_H

#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_SCRATCH_BUFFER_SIZE (10240)

#define GLOBAL_USER_CONTEXT_SERVER_CONTEXT 0
#define GLOBAL_USER_CONTEXT_WS_KEEP_ALIVE 1
#define GLOBAL_USER_CONTEXT_COUNT 2
#define DEFAULT_HTTP_SERVER_CONTEXT_SIZE    (-1)

typedef struct {
    char fs_base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[HTTP_SCRATCH_BUFFER_SIZE + 1];
} http_server_context_t;

extern esp_err_t http_server_main( size_t http_server_context_size, int max_uri_handlers );

extern void *http_get_server_context( httpd_handle_t handle, int context_id );

extern esp_err_t http_register_uri_handler( httpd_handle_t handle,
                                            const char *log_tag,
                                            const httpd_uri_t *uri_handler );

#ifdef __cplusplus
}
#endif

#endif //LIB_REST_SERVER_H
