#ifndef HAJO_HTTP_EVENTS_H
#define HAJO_HTTP_EVENTS_H


#include <esp_event.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(HTTP_SERVER_EVENT);

typedef enum {
    HTTP_SERVER_EVENT_SERVER_START = 0,
    HTTP_SERVER_EVENT_SERVER_STOP,
    HTTP_SERVER_EVENT_FILE_DESCRIPTOR_OPEN,
    HTTP_SERVER_EVENT_FILE_DESCRIPTOR_CLOSE,
} http_server_event_id_t;

typedef struct {
    httpd_handle_t hd;
    const char* ssid;
} http_server_server_event_data;

typedef struct {
    httpd_handle_t hd;
    int sockfd;
} http_server_file_descriptor_event_data;

#ifdef __cplusplus
}
#endif

#endif //HAJO_HTTP_EVENTS_H
