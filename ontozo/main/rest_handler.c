#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "lib/rest_server.h"
#include "rest_handler.h"
#include "gpio_logic.h"

static esp_err_t state_get_handler( httpd_req_t *req ) {
    httpd_resp_set_type( req, "application/json" );
    char as_string[16];
    cJSON *root = cJSON_CreateObject();
    cJSON *sensors = cJSON_AddObjectToObject( root, "levels" );
    for ( int i = 0; i < gpio_get_number_of_levels(); i++ ) {
        itoa( i + 1, as_string, sizeof as_string );
        cJSON_AddBoolToObject( sensors, as_string, gpio_get_level_state( i ));
    }

    cJSON *zones = cJSON_AddObjectToObject( root, "zones" );
    int zone_count = gpio_get_number_of_zones();
    for ( int i = 0; i < zone_count; i++ ) {
        itoa( i + 1, as_string, sizeof as_string );
        cJSON *zone = cJSON_AddObjectToObject( zones, as_string );
        cJSON_AddStringToObject( zone, "name", gpio_get_zone_name( i ));
        cJSON_AddBoolToObject( zone, "on", gpio_get_zone_state( i ));
    }

    cJSON *pumps = cJSON_AddObjectToObject( root, "pumps" );
    int pump_count = gpio_get_number_of_pumps();
    for ( int i = 0; i < pump_count; i++ ) {
        itoa( i + 1, as_string, sizeof as_string );
        cJSON *pump = cJSON_AddObjectToObject( pumps, as_string );
        cJSON_AddStringToObject( pump, "name", gpio_get_pump_name( i ));
        cJSON_AddBoolToObject( pump, "on", gpio_get_pump_state( i ));
    }

    const char *json_response = cJSON_Print( root );
    httpd_resp_sendstr( req, json_response );
    free((void *) json_response );
    cJSON_Delete( root );
    return ESP_OK;
}

static esp_err_t zones_put_handler_inner( httpd_req_t *req, cJSON *root ) {
    cJSON *id_element = cJSON_GetObjectItem( root, "id" );
    if ( id_element == NULL) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is mandatory" );
        return ESP_FAIL;
    }
    int id = id_element->valueint;
    if ( !gpio_is_zone_valid( id )) {
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is not valid" );
        return ESP_FAIL;
    }

    cJSON *state_element = cJSON_GetObjectItem( root, "state" );
    if ( state_element != NULL) {
        int state = state_element->valueint;
        gpio_set_zone_state( id - 1, state );
        ESP_LOGI( REST_TAG, "Zone %d state changed to %d", id, state );
    }
    cJSON *name_element = cJSON_GetObjectItem( root, "name" );
    if ( name_element != NULL) {
        gpio_set_zone_name( id - 1, name_element->valuestring );
        ESP_LOGI( REST_TAG, "Zone %d name changed", id );
    }
    httpd_resp_sendstr( req, "Zone changed successfully" );
    return ESP_OK;
}

static esp_err_t zones_put_handler( httpd_req_t *req ) {
    cJSON *root;
    esp_err_t result;

    if (( result = rest_receive_json_body( req, &root )) != ESP_OK ) {
        return result;
    }
    result = zones_put_handler_inner( req, root );
    cJSON_Delete( root );
    return result;
}

static esp_err_t pumps_put_handler( httpd_req_t *req ) {
    return ESP_ERR_NOT_SUPPORTED;
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