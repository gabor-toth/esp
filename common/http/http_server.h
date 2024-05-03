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

#define GLOBAL_USER_CONTEXT_REST_CONTEXT 0
#define GLOBAL_USER_CONTEXT_WS_KEEP_ALIVE 1
#define GLOBAL_USER_CONTEXT_COUNT 2

typedef struct {
    char fs_base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[HTTP_SCRATCH_BUFFER_SIZE];
} http_server_context_t;

typedef esp_err_t (*http_wifi_connect_func_t)(httpd_handle_t server, const char *wifi_ssid );

typedef void (*http_wifi_disconnect_func_t)(httpd_handle_t server );

typedef struct {
    const char *name;
    http_wifi_connect_func_t wifi_connect_fn;
    http_wifi_disconnect_func_t wifi_disconnect_fn;
    httpd_open_func_t open_fn;
    httpd_close_func_t close_fn;
} http_callbacks_t;

typedef esp_err_t (*http_register_handlers_t)(httpd_handle_t server,
                                              http_server_context_t * );

extern esp_err_t http_register_callbacks(const http_callbacks_t *callbacks );

extern esp_err_t http_server_main();

extern esp_err_t http_register_static_files_handler(httpd_handle_t server,
                                                    http_server_context_t *http_context,
                                                    const char *static_files_base_path );

extern esp_err_t http_register_uri_handler(httpd_handle_t handle,
                                           const char *log_tag,
                                           const httpd_uri_t *uri_handler );

#ifdef __cplusplus
}
#endif

#endif //LIB_REST_SERVER_H
