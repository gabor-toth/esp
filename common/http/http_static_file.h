#ifndef HAJO_HTTP_STATIC_FILE_H
#define HAJO_HTTP_STATIC_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "http_server.h"

extern void http_static_files_register();

extern esp_err_t http_static_files_register_handler( httpd_handle_t server,
                                                     http_server_context_t *http_context,
                                                     const char *static_files_base_path );

#ifdef __cplusplus
}
#endif

#endif //HAJO_HTTP_STATIC_FILE_H
