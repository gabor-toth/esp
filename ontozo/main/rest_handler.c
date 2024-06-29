#include "esp_http_server.h"
#include "gpio_rest.h"
#include "http/http_server.h"
#include "http/http_events.h"
#include "http/http_static_file.h"
#include "nvs_main.h"
#include "program_logic_rest.h"
#include "program_rest.h"
#include "rest_handler.h"
#include "rest_util.h"

static esp_err_t options_handler( httpd_req_t *req ) {
    rest_allow_cors( req );
    httpd_resp_set_hdr( req, "Access-Control-Allow-Methods", "PUT,POST,DELETE" );
    httpd_resp_sendstr( req, "" );
    return ESP_OK;
}

static void rest_register_options_handlers( httpd_handle_t server, http_server_context_t *server_context ) {
    httpd_uri_t options_uri = {
            .uri = "/*",
            .method = HTTP_OPTIONS,
            .handler = options_handler,
            .user_ctx = server_context
    };
    httpd_register_uri_handler( server, &options_uri );
}

static void
handler_on_http_server_start( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    http_server_server_event_data *data = event_data;
    http_server_context_t *server_context = http_get_server_context(data->hd, GLOBAL_USER_CONTEXT_SERVER_CONTEXT);
    
    rest_register_options_handlers( data->hd, server_context );
    rest_register_gpio_handlers( data->hd, server_context );
    rest_register_programs_handlers( data->hd, server_context );
    rest_register_program_logic_handlers( data->hd, server_context );
    http_register_static_files_handler(data->hd, server_context, "");
}

void rest_register() {
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START,
                                        &handler_on_http_server_start, NULL ) );
}
