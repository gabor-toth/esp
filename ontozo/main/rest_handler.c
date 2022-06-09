#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "lib/nvs_main.h"
#include "lib/rest_server.h"
#include "lib/sntp_main.h"
#include "gpio_rest.h"
#include "program_rest.h"
#include "rest_handler.h"

static const char *LOG_TAG = "rest_handler";

static esp_err_t options_handler( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_set_hdr( req, "Access-Control-Allow-Methods", "PUT,POST,DELETE" );
    httpd_resp_sendstr( req, "" );
    return ESP_OK;
}


static void rest_register_options_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t options_uri = {
            .uri = "/*",
            .method = HTTP_OPTIONS,
            .handler = options_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &options_uri );
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    rest_register_options_handlers( server, rest_context );
    rest_register_gpio_handlers( server, rest_context );
    rest_register_programs_handlers( server, rest_context );
}
