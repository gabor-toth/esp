#ifndef HAJO_HTTPREQUEST_H
#define HAJO_HTTPREQUEST_H

#include "esp_http_client.h"

class HttpRequest {
public:
    HttpRequest();

    ~HttpRequest();

    void setGetRequest( const char *url );

    void setPostRequest( const char *url, const char *postData );

    esp_err_t send();

    void release();

    esp_err_t eventHandler( esp_http_client_event_t *evt );

private:
    char *buffer;
    char *postData;
    char *url;
    esp_http_client_handle_t client;
    esp_http_client_method_t method;
    int bufferSize;
    int length;
    int status_code;

    esp_err_t receivedBytes( void *data, int data_length, int content_length );

public:
    [[nodiscard]] char *getBuffer() const { return buffer; };

    [[nodiscard]] int getBufferLength() const { return length; };

    [[nodiscard]] int getStatusCode() const { return status_code; };

    [[nodiscard]] int getMethod() const { return method; };

    [[nodiscard]] const char *getUrl() const { return url; };
};


#endif //HAJO_HTTPREQUEST_H
