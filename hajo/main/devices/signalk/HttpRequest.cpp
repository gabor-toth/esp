#include <algorithm>
#include "HttpRequest.h"
#include "esp_log.h"

static const char *TAG = "httpclient";

static esp_err_t http_event_handler( esp_http_client_event_t *evt ) {
    HttpRequest *http_client_data = static_cast<HttpRequest *>(evt->user_data);
    return http_client_data->eventHandler( evt );
}

HttpRequest::HttpRequest() {
    buffer = nullptr;
    bufferSize = 0;
    client = nullptr;
    length = 0;
    method = HTTP_METHOD_MAX;
    postData = nullptr;
    status_code = 0;
    url = nullptr;
}

HttpRequest::~HttpRequest() {
    release();
}

void HttpRequest::release() {
    ESP_LOGI( TAG,"release",  );
//    if ( client != nullptr ) {
//        esp_http_client_handle_t _client = client;
//        client = nullptr;
//        esp_http_client_cleanup( _client );
//    }
    if ( buffer != nullptr ) {
        free( buffer );
        buffer = nullptr;
        length = bufferSize = 0;
    }
    if ( url != nullptr ) {
        free( url );
        url = nullptr;
    }
    if ( postData != nullptr ) {
        free( postData );
        postData = nullptr;
    }
}

void HttpRequest::setGetRequest( const char* _url ) {
    release();

    this->url = strdup( _url );
    this->method = HTTP_METHOD_GET;
    ESP_LOGI( TAG,"GET %s", url );
}


void HttpRequest::setPostRequest( const char* _url, const char* _postData ) {
    release();

    this->url = strdup( _url );
    this->postData = strdup( _postData );
    this->method = HTTP_METHOD_POST;
    ESP_LOGI( TAG,"POST %s", url );
}

esp_err_t HttpRequest::send() {
    length = 0;
    esp_http_client_config_t config = {
            .url = url,
            .host = nullptr,
            .port = 0,
            .username = nullptr,
            .password = nullptr,
            .auth_type = HTTP_AUTH_TYPE_NONE,
            .path = nullptr,
            .query = nullptr,
            .cert_pem = nullptr,
            .cert_len = 0,
            .client_cert_pem = nullptr,
            .client_cert_len = 0,
            .client_key_pem = nullptr,
            .client_key_len = 0,
            .client_key_password = nullptr,
            .client_key_password_len = 0,
            .tls_version = ESP_HTTP_CLIENT_TLS_VER_ANY,
            .user_agent = nullptr,
            .method = method,
            .timeout_ms = 0,
            .disable_auto_redirect = true,
            .max_redirection_count = 0,
            .max_authorization_retries = 0,
            .event_handler = http_event_handler,
            .transport_type= HTTP_TRANSPORT_UNKNOWN,
            .buffer_size = 0,
            .buffer_size_tx = 0,
            .user_data = this,
            .is_async = false,
            .use_global_ca_store = false,
            .skip_cert_common_name_check = false,
            .common_name = nullptr,
            .crt_bundle_attach = nullptr,
            .keep_alive_enable = false,
            .keep_alive_idle = 0,
            .keep_alive_interval = 0,
            .keep_alive_count = 0,
            .if_name = nullptr,
    };
    client = esp_http_client_init( &config );
    if ( client == nullptr ) {
        return ESP_FAIL;
    }
    if ( postData != nullptr) {
        esp_http_client_set_header( client, "Content-Type", "application/json" );
        esp_http_client_set_post_field( client, postData, (int)strlen( postData ) );
    }
    esp_err_t result = esp_http_client_perform( client );
    esp_http_client_cleanup( client );
    return result;
}

esp_err_t HttpRequest::eventHandler( esp_http_client_event_t *evt ) {
    switch ( evt->event_id ) {
        case HTTP_EVENT_ON_DATA: {
            status_code = esp_http_client_get_status_code( evt->client );
            bool chunked = esp_http_client_is_chunked_response( evt->client );
//            ESP_LOGI( TAG, "HTTP_EVENT_ON_DATA content_len=%lld data_len=%d chunked=%d",
//                      esp_http_client_get_content_length( evt->client ), evt->data_len, chunked );
            if ( !chunked ) {
                int content_length = (int) esp_http_client_get_content_length( evt->client );
                receivedBytes( evt->data, evt->data_len, content_length );
            } else {
                char *p = static_cast<char *>(malloc( evt->data_len + 1 ));
                if ( p == nullptr ) {
                    ESP_LOGE( TAG, "Failed to allocate memory for output buffer" );
                    return ESP_FAIL;
                }
                memcpy( p, evt->data, evt->data_len );
                p[ evt->data_len ] = 0;
//                ESP_LOGI( TAG, "chunk %s", p );
                free( p );
            }
            break;
        }
        case HTTP_EVENT_DISCONNECTED:
//            ESP_LOGI( TAG, "HTTP_EVENT_DISCONNECTED" );
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGI( TAG, "HTTP_EVENT_ERROR" );
            break;
        case HTTP_EVENT_ON_CONNECTED:
//            ESP_LOGI( TAG, "HTTP_EVENT_ON_CONNECTED" );
            break;
        case HTTP_EVENT_ON_FINISH:
//            ESP_LOGI( TAG, "HTTP_EVENT_ON_FINISH" );
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t HttpRequest::receivedBytes( void *data, int data_length, int content_length ) {
    int copy_len;
    if ( buffer == nullptr || content_length > bufferSize ) {
        if ( buffer != nullptr ) {
            free( buffer );
        }
        buffer = (char *) malloc( content_length + 1 );
        length = 0;
        bufferSize = content_length;
        if ( buffer == nullptr ) {
            ESP_LOGE( TAG, "Failed to allocate memory for output buffer" );
            return ESP_FAIL;
        }
    }
    copy_len = std::min( data_length, ( content_length - length ));
    if ( copy_len ) {
        memcpy( buffer + length, data, copy_len );
    }
    length += copy_len;
    buffer[ length ] = 0;
    return ESP_OK;
}