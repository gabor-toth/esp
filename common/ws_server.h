#ifndef HAJO_WS_SERVER_H
#define HAJO_WS_SERVER_H

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void wss_register();

extern void wss_server_send_message( httpd_handle_t hd, const char *message );

#ifdef __cplusplus
}
#endif

#endif //HAJO_WS_SERVER_H
