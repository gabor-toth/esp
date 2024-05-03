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

#define GLOBAL_USER_CONTEXT_REST_CONTEXT 0
#define GLOBAL_USER_CONTEXT_WS_KEEP_ALIVE 1
#define GLOBAL_USER_CONTEXT_COUNT 2

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[REST_SCRATCH_BUFSIZE];
} rest_server_context_t;

typedef esp_err_t (*rest_wifi_connect_func_t)( httpd_handle_t server, const char *wifi_ssid );

typedef void (*rest_wifi_disconnect_func_t)( httpd_handle_t server );

typedef struct {
    const char *name;
    rest_wifi_connect_func_t wifi_connect_fn;
    rest_wifi_disconnect_func_t wifi_disconnect_fn;
    httpd_open_func_t open_fn;
    httpd_close_func_t close_fn;
} rest_callbacks_t;

typedef esp_err_t (*rest_register_handlers_t)( httpd_handle_t server,
                                               rest_server_context_t * );

extern esp_err_t rest_register_callbacks( const rest_callbacks_t *callbacks );

extern esp_err_t rest_server_main();

extern esp_err_t rest_register_static_files_handler( httpd_handle_t server,
                                                     rest_server_context_t *rest_context,
                                                     const char *static_files_base_path );

extern esp_err_t rest_register_uri_handler( httpd_handle_t handle,
                                            const char *log_tag,
                                            const httpd_uri_t *uri_handler );

#ifdef __cplusplus
}
#endif

#endif //LIB_REST_SERVER_H
