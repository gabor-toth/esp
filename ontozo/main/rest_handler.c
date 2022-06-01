#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "lib/gpio_define.h"
#include "lib/nvs_main.h"
#include "lib/rest_server.h"
#include "gpio_logic.h"
#include "rest_handler.h"

typedef struct {
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

    const char *json_response = cJSON_Print( root );
    httpd_resp_sendstr( req, json_response );
    free((void *) json_response );
    cJSON_Delete( root );
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
        ESP_LOGI( REST_TAG, "%s %d state changed to %d", class_name, pin_index + 1, state );
    }
    cJSON *name_element = cJSON_GetObjectItem( root, "name" );
    if ( name_element != NULL) {
        changed = true;
        char *name = name_element->valuestring;
        gpio_set_pin_name( is_input, class, pin_index, name );

        char nvs_key[256];
        snprintf( nvs_key, sizeof nvs_key, "%s.%d.name", gpio_get_class_name( is_input, class ), pin_index + 1 );
        nvs_write_string( nvs_key, name );

        ESP_LOGI( REST_TAG, "%s %d name changed to %s", class_name, pin_index + 1, name );
    }
    if ( !changed ) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Neither state nor name changed" );
        return ESP_FAIL;
    }

    char response[256];
    snprintf( response, sizeof response, "%s changed successfully", class_name );
    httpd_resp_sendstr( req, response );
    return ESP_OK;
}

static esp_err_t pins_put_handler( httpd_req_t *req ) {
    cJSON *root;
    esp_err_t result;

    if (( result = rest_receive_json_body( req, &root )) != ESP_OK ) {
        return result;
    }
    PinHandlerContext *context = (PinHandlerContext *) req->user_ctx;
    result = pins_put_handler_inner( req, root, context->type, context->class );
    cJSON_Delete( root );
    return result;
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
            context->type = type;
            context->class = class;
            httpd_uri_t uri_definition = {
                    .uri = uri,
                    .method = HTTP_PUT,
                    .handler = pins_put_handler,
                    .user_ctx = context
            };
            httpd_register_uri_handler( server, &uri_definition );
        }
    }
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    rest_register_state_handler( server, rest_context );
    rest_register_gpio_handlers( server, rest_context );
}
