#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "lib/rest_server.h"
#include "rest_handler.h"
#include "lib/gpio_define.h"
#include "gpio_logic.h"

static esp_err_t state_get_handler( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_set_type( req, "application/json" );
    char as_string[16];
    cJSON *root = cJSON_CreateObject();

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        cJSON *typeJson = cJSON_AddObjectToObject( root, type == INPUTS ? "inputs" : "outputs" );

        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            cJSON *class_json = cJSON_AddObjectToObject( typeJson, gpio_get_class_name( type, class ));
            int pin_count = gpio_get_number_of_pins( type, class );
            for ( int i = 0; i < pin_count; i++ ) {
                itoa( i + 1, as_string, sizeof as_string );
                cJSON *zone = cJSON_AddObjectToObject( class_json, as_string );
                cJSON_AddStringToObject( zone, "name", gpio_get_pin_name( type, class, i ));
                cJSON_AddBoolToObject( zone, "on", gpio_get_pin_state( type, class, i ));
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
    int id = id_element->valueint;
    if ( !gpio_is_valid_index( is_input, class, id )) {
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
        gpio_set_pin_state( is_input, class, id - 1, state );
        ESP_LOGI( REST_TAG, "%s %d state changed to %d", class_name, id, state );
    }
    cJSON *name_element = cJSON_GetObjectItem( root, "name" );
    if ( name_element != NULL) {
        changed = true;
        char *name = name_element->valuestring;
        gpio_set_pin_name( is_input, class, id - 1, name );
        ESP_LOGI( REST_TAG, "%s %d name changed to %s", class_name, id, name );
    }
    if ( changed ) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Neither state nor name changed" );
        return ESP_FAIL;
    }

    char response[256];
    snprintf( response, sizeof response, "%s changed successfully", class_name );
    httpd_resp_sendstr( req, response );
    return ESP_OK;
}

static esp_err_t pins_put_handler( httpd_req_t *req, bool is_input, int class ) {
    cJSON *root;
    esp_err_t result;

    if (( result = rest_receive_json_body( req, &root )) != ESP_OK ) {
        return result;
    }
    result = pins_put_handler_inner( req, root, is_input, class );
    cJSON_Delete( root );
    return result;
}

static esp_err_t zones_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, OUTPUTS, ZONES );
}

static esp_err_t pumps_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, OUTPUTS, PUMPS );
}

static esp_err_t levels_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, INPUTS, LEVELS );
}

static esp_err_t buttons_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, INPUTS, BUTTONS );
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t state_get_uri = {
            .uri = "/state",
            .method = HTTP_GET,
            .handler = state_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &state_get_uri );

    httpd_uri_t zones_put_uri = {
            .uri = "/zones",
            .method = HTTP_PUT,
            .handler = zones_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &zones_put_uri );

    httpd_uri_t pumps_put_uri = {
            .uri = "/pumps",
            .method = HTTP_PUT,
            .handler = pumps_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &pumps_put_uri );

    httpd_uri_t levels_put_uri = {
            .uri = "/levels",
            .method = HTTP_PUT,
            .handler = levels_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &levels_put_uri );

    httpd_uri_t buttons_put_uri = {
            .uri = "/buttons",
            .method = HTTP_PUT,
            .handler = buttons_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &buttons_put_uri );
}