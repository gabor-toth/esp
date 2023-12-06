#ifndef HAJO_HTTPREQUEST_H
#define HAJO_HTTPREQUEST_H

#include "esp_http_client.h"

class HttpRequest {
public:
    HttpRequest();

    ~HttpRequest();

    char *buffer;
    int length;

    void setGetRequest( const char *url );

    void setPostRequest( const char *url, const char *postData );

    esp_err_t send();

    void release();

    esp_err_t eventHandler( esp_http_client_event_t *evt );

private:
    esp_http_client_handle_t client;
    int bufferSize;
    esp_http_client_method_t method;
    char *url;
    char *postData;

    esp_err_t receivedBytes( void *data, int data_length, int content_length );

};


#endif //HAJO_HTTPREQUEST_H
