#include "sdkconfig.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)
#include "cJSON.h"
#include "rest_util.h"
#include "sntp_main.h"

void rest_set_json_content_type( httpd_req_t *req ) {
    httpd_resp_set_type( req, HTTPD_TYPE_JSON );
}

esp_err_t rest_set_error_code( httpd_req_t *req, esp_err_t esp_err, const char *message ) {
    httpd_err_code_t http_error;

    switch ( esp_err ) {
        case ESP_ERR_INVALID_ARG:
            http_error = HTTPD_400_BAD_REQUEST;
            break;
        case ESP_ERR_NOT_FOUND:
            http_error = HTTPD_404_NOT_FOUND;
            break;
        default:
            http_error = HTTPD_500_INTERNAL_SERVER_ERROR;
            break;
    }
    return httpd_resp_send_err( req, http_error, message );
}

esp_err_t rest_parse_index( const char *uri, int *index, bool needed ) {
    char *end;
    *index = strtol( uri, &end, 10 );
    if ( *end != 0 || ( needed && end == uri )) {
        printf( "Numeric index expected\n" );
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

void rest_send_message_back( httpd_req_t *req, const char *format, ... ) {
    char message[256];
    char response[300];
    va_list args;

    va_start ( args, format );
    vsnprintf( message, sizeof message, format, args );
    va_end ( args );

    snprintf( response, sizeof response, "{ \"message\": \"%s\" }", message );
    rest_set_json_content_type( req );
    httpd_resp_sendstr( req, response );
}

void rest_send_json_back( httpd_req_t *req, cJSON *root ) {
    const char *json_response = cJSON_Print( root );
    cJSON_Delete( root );
    rest_set_json_content_type( req );
    httpd_resp_sendstr( req, json_response );
    free((void *) json_response );
}

void rest_add_time_json( cJSON *root ) {
    char time_buf[64];
    local_time_to_buf( time_buf, sizeof time_buf );
    cJSON *timeJson = cJSON_AddObjectToObject( root, "time" );
    cJSON_AddStringToObject( timeJson, "time", time_buf );
    cJSON_AddBoolToObject( timeJson, "isTimeSet", sntp_is_time_set());
}

void rest_allow_cors( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
}

esp_err_t rest_receive_json_body(httpd_req_t *req, http_server_context_t *context, cJSON **root ) {
    *root = 0;

    size_t total_len = req->content_len;
    int cur_len = 0;
    char *buf = context->scratch;
    if (total_len >= HTTP_SCRATCH_BUFFER_SIZE) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "content too long" );
        return ESP_FAIL;
    }
    while ( cur_len < total_len ) {
        int received = httpd_req_recv( req, buf + cur_len, total_len );
        if ( received <= 0 ) {
            httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value" );
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[ total_len ] = '\0';

    *root = cJSON_Parse( buf );
    return ESP_OK;
}

#endif
