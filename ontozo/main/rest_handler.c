#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "lib/rest_server.h"
#include "rest_handler.h"
#include "gpio_logic.h"

static esp_err_t state_get_handler( httpd_req_t *req ) {
    httpd_resp_set_hdr( req, "Access-Control-Allow-Origin", "*" );
    httpd_resp_set_type( req, "application/json" );
    char as_string[16];
    cJSON *root = cJSON_CreateObject();
    cJSON *sensors = cJSON_AddObjectToObject( root, "levels" );
    for ( int i = 0; i < gpio_get_number_of_levels(); i++ ) {
        itoa( i + 1, as_string, sizeof as_string );
        cJSON_AddBoolToObject( sensors, as_string, gpio_get_level_state( i ));
    }

    int class_count = gpio_get_number_of_output_classes();
    for ( int class = 0; class < class_count; class++ ) {
        cJSON *class_json = cJSON_AddObjectToObject( root, gpio_get_class_name( class ));
        int pin_count = gpio_get_number_of_output_pins( class );
        for ( int i = 0; i < pin_count; i++ ) {
            itoa( i + 1, as_string, sizeof as_string );
            cJSON *zone = cJSON_AddObjectToObject( class_json, as_string );
            cJSON_AddStringToObject( zone, "name", gpio_get_output_pin_name( class, i ));
            cJSON_AddBoolToObject( zone, "on", gpio_get_output_pin_state( class, i ));
        }
    }

    const char *json_response = cJSON_Print( root );
    httpd_resp_sendstr( req, json_response );
    free((void *) json_response );
    cJSON_Delete( root );
    return ESP_OK;
}

static esp_err_t pins_put_handler_inner( httpd_req_t *req, cJSON *root, int class ) {
    cJSON *id_element = cJSON_GetObjectItem( root, "id" );
    if ( id_element == NULL) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is mandatory" );
        return ESP_FAIL;
    }
    int id = id_element->valueint;
    if ( !gpio_is_valid_output_index( ZONES, id )) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is not valid" );
        return ESP_FAIL;
    }

    cJSON *state_element = cJSON_GetObjectItem( root, "state" );
    char *class_name = gpio_get_class_name( class );
    if ( state_element != NULL) {
        int state = state_element->valueint;
        gpio_set_output_pin_state( class, id - 1, state );
        ESP_LOGI( REST_TAG, "%s %d state changed to %d", class_name, id, state );
    }
    cJSON *name_element = cJSON_GetObjectItem( root, "name" );
    if ( name_element != NULL) {
        gpio_set_output_pin_name( class, id - 1, name_element->valuestring );
        ESP_LOGI( REST_TAG, "%s %d name changed", class_name, id );
    }

    char response[256];
    snprintf( response, sizeof response, "%s changed successfully", class_name );
    httpd_resp_sendstr( req, response );
    return ESP_OK;
}

static esp_err_t pins_put_handler( httpd_req_t *req, int class ) {
    cJSON *root;
    esp_err_t result;

    if (( result = rest_receive_json_body( req, &root )) != ESP_OK ) {
        return result;
    }
    result = pins_put_handler_inner( req, root, class );
    cJSON_Delete( root );
    return result;
}

static esp_err_t zones_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, ZONES );
}

static esp_err_t pumps_put_handler( httpd_req_t *req ) {
    return pins_put_handler( req, PUMPS );
}

void rest_register_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    /* URI handler for fetching temperature data */
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
}