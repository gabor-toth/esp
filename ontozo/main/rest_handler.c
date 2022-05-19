#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "lib/rest_server.h"
#include "rest_handler.h"


/* Simple handler for light brightness control */
static esp_err_t light_brightness_post_handler( httpd_req_t *req ) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *) ( req->user_ctx ))->scratch;
    int received = 0;
    if ( total_len >= REST_SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long" );
        return ESP_FAIL;
    }
    while ( cur_len < total_len ) {
        received = httpd_req_recv( req, buf + cur_len, total_len );
        if ( received <= 0 ) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value" );
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[ total_len ] = '\0';

    cJSON *root = cJSON_Parse( buf );
    int red = cJSON_GetObjectItem( root, "red" )->valueint;
    int green = cJSON_GetObjectItem( root, "green" )->valueint;
    int blue = cJSON_GetObjectItem( root, "blue" )->valueint;
    ESP_LOGI( REST_TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue );
    cJSON_Delete( root );
    httpd_resp_sendstr( req, "Post control value successfully" );
    return ESP_OK;
}

/* Simple handler for getting temperature data */
static esp_err_t temperature_data_get_handler( httpd_req_t *req ) {
    httpd_resp_set_type( req, "application/json" );
    cJSON *root = cJSON_CreateObject();
//    cJSON_AddNumberToObject( root, "raw", esp_random() % 20 );
    const char *sys_info = cJSON_Print( root );
    httpd_resp_sendstr( req, sys_info );
    free((void *) sys_info );
    cJSON_Delete( root );
    return ESP_OK;
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    /* URI handler for fetching temperature data */
    httpd_uri_t temperature_data_get_uri = {
            .uri = "/temp/raw",
            .method = HTTP_GET,
            .handler = temperature_data_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &temperature_data_get_uri );

    /* URI handler for light brightness control */
    httpd_uri_t light_brightness_post_uri = {
            .uri = "/light/brightness",
            .method = HTTP_POST,
            .handler = light_brightness_post_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &light_brightness_post_uri );
}