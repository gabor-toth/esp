#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "lib/gpio_define.h"
#include "lib/nvs_main.h"
#include "lib/rest_server.h"
#include "lib/sntp_main.h"
#include "gpio_logic.h"
#include "program_json.h"
#include "rest_handler.h"

static const char *LOG_TAG = "rest-handler";

typedef struct {
    rest_server_context_t *rest_context;
    bool type;
    int class;
} PinHandlerContext;

static esp_err_t state_get_handler( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_set_type( req, "application/json" );
    cJSON *root = cJSON_CreateObject();

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        cJSON *typeJson = cJSON_AddObjectToObject( root, type == INPUTS ? "inputs" : "outputs" );

        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            cJSON *itemArray = cJSON_AddArrayToObject( typeJson, gpio_get_class_name( type, class ));
            int pin_count = gpio_get_number_of_pins( type, class );
            for ( int i = 0; i < pin_count; i++ ) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddNumberToObject( item, "id", i + 1 );
                cJSON_AddStringToObject( item, "name", gpio_get_pin_name( type, class, i ));
                cJSON_AddBoolToObject( item, "on", gpio_get_pin_state( type, class, i ));
                cJSON_AddItemToArray( itemArray, item );
            }
        }
    }

    char time_buf[64];
    local_time_to_buf( time_buf, sizeof time_buf );
    cJSON *timeJson = cJSON_AddObjectToObject( root, "time" );
    cJSON_AddStringToObject( timeJson, "time", time_buf );
    cJSON_AddBoolToObject( timeJson, "isTimeSet", sntp_is_time_set());

    const char *json_response = cJSON_Print( root );
    cJSON_Delete( root );
    httpd_resp_sendstr( req, json_response );
    free((void *) json_response );
    return ESP_OK;
}

static esp_err_t pins_put_handler_inner( httpd_req_t *req, cJSON *root, bool is_input, int class ) {
    cJSON *id_element = cJSON_GetObjectItem( root, "id" );
    if ( id_element == NULL) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is mandatory" );
        return ESP_FAIL;
    }
    int pin_index = id_element->valueint - 1;
    if ( !gpio_is_valid_index( is_input, class, pin_index )) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is not valid" );
        return ESP_FAIL;
    }

    cJSON *state_element = cJSON_GetObjectItem( root, "state" );
    char *class_name = gpio_get_class_name( is_input, class );
    bool changed = false;
    if ( state_element != NULL) {
        changed = true;
        if ( is_input ) {
            httpd_resp_send_err( req, HTTPD_403_FORBIDDEN, "Unable to set state of an input pin" );
            return ESP_FAIL;
        }
        int state = state_element->valueint;
        gpio_set_pin_state( is_input, class, pin_index, state );
        ESP_LOGI( LOG_TAG, "%s %d state changed to %d", class_name, pin_index + 1, state );
    }
    cJSON *name_element = cJSON_GetObjectItem( root, "name" );
    if ( name_element != NULL) {
        changed = true;
        char *name = name_element->valuestring;
        gpio_set_pin_name( is_input, class, pin_index, name );

        char nvs_key[256];
        snprintf( nvs_key, sizeof nvs_key, "%s.%d.name", gpio_get_class_name( is_input, class ), pin_index + 1 );
        nvs_write_string( nvs_key, name );

        ESP_LOGI( LOG_TAG, "%s %d name changed to %s", class_name, pin_index + 1, name );
    }
    if ( !changed ) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Neither state nor name changed" );
        return ESP_FAIL;
    }

    char response[256];
    snprintf( response, sizeof response, "{ \"message\": \"%s changed successfully\" }", class_name );
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_sendstr( req, response );
    return ESP_OK;
}

static esp_err_t pins_put_handler( httpd_req_t *req ) {
    cJSON *root;
    esp_err_t result;

    result = rest_receive_json_body( req, ((PinHandlerContext *) req->user_ctx )->rest_context, &root );
    if ( result != ESP_OK ) {
        return result;
    }
    PinHandlerContext *context = (PinHandlerContext *) req->user_ctx;
    result = pins_put_handler_inner( req, root, context->type, context->class );
    cJSON_Delete( root );
    return result;
}

static esp_err_t program_put_handler( httpd_req_t *req ) {
    esp_err_t result;

    cJSON *root;
    result = rest_receive_json_body( req, (rest_server_context_t *) req->user_ctx, &root );
    if ( result != ESP_OK ) {
        return result;
    }

    Program *program;
    result = program_read_from_json( root, &program );
    if ( result != ESP_OK ) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Bad request, see log for more information" );
        return ESP_FAIL;
    }

    char *json_response;
    program_write_to_json( program, &json_response );
    program_destructor( program );

    httpd_resp_set_hdr( req, "Content-Type", HTTPD_TYPE_JSON );
    httpd_resp_sendstr( req, json_response );
    free( json_response );

    return ESP_OK;
}

static esp_err_t options_handler( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_set_hdr( req, "Access-Control-Allow-Methods", "PUT" );
    httpd_resp_sendstr( req, "" );
    return ESP_OK;
}


static void rest_register_state_handler( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t state_get_uri = {
            .uri = "/state",
            .method = HTTP_GET,
            .handler = state_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &state_get_uri );
}

static void rest_register_gpio_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    char uri[256];

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            snprintf( uri, sizeof uri, "/%s", gpio_get_class_name( type, class ));
            PinHandlerContext *context = malloc( sizeof( PinHandlerContext ));
            context->rest_context = rest_context;
            context->type = type;
            context->class = class;
            httpd_uri_t uri_definition = {
                    .uri = uri,
                    .method = HTTP_PUT,
                    .handler = pins_put_handler,
                    .user_ctx = context
            };
            esp_err_t err = httpd_register_uri_handler( server, &uri_definition );
            if ( err != ESP_OK ) {
                ESP_LOGE( LOG_TAG, "Failed to register URI handlerPUT %s: %s", uri, esp_err_to_name( err ));
            } else {
                ESP_LOGI( LOG_TAG, "Registered PUT %s", uri );
            }
        }
    }
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

static void rest_register_programs_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t program_put_uri = {
            .uri = "/programs",
            .method = HTTP_PUT,
            .handler = program_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &program_put_uri );
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    rest_register_state_handler( server, rest_context );
    rest_register_gpio_handlers( server, rest_context );
    rest_register_options_handlers( server, rest_context );
    rest_register_programs_handlers( server, rest_context );
}
