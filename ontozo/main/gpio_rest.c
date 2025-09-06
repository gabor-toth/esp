#include <esp_log.h>
#include "gpio_define.h"
#include "gpio_rest.h"
#include "nvs_main.h"
#include "sntp_main.h"
#include "rest_util.h"

#define PINS_PREFIX    "/pins"

static const char *LOG_TAG = "gpio_rest";

typedef struct {
    http_server_context_t *server_context;
    bool type;
    int class;
} PinHandlerContext;

static esp_err_t state_get_handler( httpd_req_t *req ) {
    rest_allow_cors( req );
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "version", gpio_get_version() );
    rest_add_time_json( root );

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        cJSON *typeJson = cJSON_AddObjectToObject( root, type == INPUTS ? "inputs" : "outputs" );

        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            cJSON *itemArray = cJSON_AddArrayToObject( typeJson, gpio_get_class_name( type, class ) );
            int pin_count = gpio_get_number_of_pins( type, class );
            for ( int i = 0; i < pin_count; i++ ) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddNumberToObject( item, "id", i + 1 );
                cJSON_AddBoolToObject( item, "on", gpio_get_pin_state( type, class, i ) );
                cJSON_AddItemToArray( itemArray, item );
            }
        }
    }

    rest_send_json_back_and_delete( req, root );
    return ESP_OK;
}

static esp_err_t pins_get_handler( httpd_req_t *req ) {
    rest_allow_cors( req );
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "version", gpio_get_version() );

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        cJSON *typeJson = cJSON_AddObjectToObject( root, type == INPUTS ? "inputs" : "outputs" );

        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            cJSON *itemArray = cJSON_AddArrayToObject( typeJson, gpio_get_class_name( type, class ) );
            int pin_count = gpio_get_number_of_pins( type, class );
            for ( int i = 0; i < pin_count; i++ ) {
                PinData pin_data;
                gpio_get_pin_data( type, class, i, &pin_data );
                cJSON *item = cJSON_CreateObject();
                cJSON_AddNumberToObject( item, "id", i + 1 );
                cJSON_AddStringToObject( item, "name", pin_data.name );
                cJSON_AddBoolToObject( item, "manual", pin_data.is_manual );
                cJSON_AddItemToArray( itemArray, item );
            }
        }
    }

    rest_send_json_back_and_delete( req, root );
    return ESP_OK;
}

static esp_err_t pins_put_handler_inner( httpd_req_t *req, cJSON *root, bool is_input, int class ) {
    cJSON *id_element = cJSON_GetObjectItem( root, "id" );
    if ( id_element == NULL ) {
        return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is mandatory" );
    }
    int pin_index = id_element->valueint - 1;
    if ( !gpio_is_valid_index( is_input, class, pin_index ) ) {
        return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Id is not valid" );
    }

    const char *class_name = gpio_get_class_name( is_input, class );

    bool changed = false;
    PinData pin_data;
    gpio_get_pin_data( is_input, class, pin_index, &pin_data );

    cJSON *element;
    element = cJSON_GetObjectItem( root, "name" );
    if ( cJSON_IsString( element ) ) {
        changed = true;
        pin_data.name = element->valuestring;
        ESP_LOGI( LOG_TAG, "%s %d name changed to %s", class_name, pin_index + 1, pin_data.name );
    }
    element = cJSON_GetObjectItem( root, "manual" );
    if ( cJSON_IsBool( element ) ) {
        changed = true;
        pin_data.is_manual = element->valueint;
        ESP_LOGI( LOG_TAG, "%s %d manual changed to %d", class_name, pin_index + 1, pin_data.is_manual );
    }
    if ( !changed ) {
        return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Nothing changed" );
    }
    gpio_set_pin_data( is_input, class, pin_index, &pin_data );

    rest_allow_cors( req );
    rest_send_message_back( req, "%s changed successfully", class_name );
    return ESP_OK;
}

static esp_err_t pins_put_config_handler( httpd_req_t *req ) {
    cJSON *root;
    esp_err_t result;

    result = rest_receive_json_body( req, ( (PinHandlerContext *) req->user_ctx )->server_context, &root );
    if ( result != ESP_OK ) {
        return result;
    }
    PinHandlerContext *context = (PinHandlerContext *) req->user_ctx;
    result = pins_put_handler_inner( req, root, context->type, context->class );
    cJSON_Delete( root );
    return result;
}

static esp_err_t pins_put_state_handler( httpd_req_t *req ) {
    PinHandlerContext *context = (PinHandlerContext *) req->user_ctx;

    const char *ptr = req->uri + strlen( PINS_PREFIX ) + 1;
    while ( *ptr && *ptr != '/' ) {
        ptr++;
    }
    if ( *ptr != '/' ) {
        ESP_LOGW( LOG_TAG, "Index expected in URL, got %s at %d", ptr, (int) ( ptr - req->uri ) );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "Index expected in URL" );
    }
    ptr++;
    int pin_index = (int) strtol( ptr, (char **) &ptr, 10 ) - 1;
    if ( *ptr != '/' ) {
        ESP_LOGW( LOG_TAG, "Index expected in URL, got %s at %d", ptr, (int) ( ptr - req->uri ) );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "Index expected in URL" );
    }
    ptr++;
    int state;
    if ( strcmp( ptr, "on" ) == 0 ) {
        state = 1;
    } else if ( strcmp( ptr, "off" ) == 0 ) {
        state = 0;
    } else {
        ESP_LOGW( LOG_TAG, "Index expected in URL, got %s at %d", ptr, (int) ( ptr - req->uri ) );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "State expected in URL" );
    }
    const char *class_name = gpio_get_class_name( context->type, context->class );
    gpio_set_pin_state_forced( context->type, context->class, pin_index, state );
    ESP_LOGI( LOG_TAG, "%s %d state changed to %d", class_name, pin_index + 1, state );

    rest_allow_cors( req );
    rest_send_message_back( req, "%s/%d changed successfully", class_name, pin_index + 1 );
    return ESP_OK;
}

static void rest_register_state_handler( httpd_handle_t server, http_server_context_t *server_context ) {
    httpd_uri_t state_get_uri = {
            .uri = PINS_PREFIX "/state",
            .method = HTTP_GET,
            .handler = state_get_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &state_get_uri ) );
}

static void rest_register_get_handler( httpd_handle_t server, http_server_context_t *server_context ) {
    httpd_uri_t state_get_uri = {
            .uri = PINS_PREFIX,
            .method = HTTP_GET,
            .handler = pins_get_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &state_get_uri ) );
}

static void rest_register_put_handlers( httpd_handle_t server, http_server_context_t *server_context ) {
    char uri[256];

    for ( int type = OUTPUTS; type <= INPUTS; type++ ) {
        int class_count = gpio_get_number_of_classes( type );
        for ( int class = 0; class < class_count; class++ ) {
            snprintf( uri, sizeof uri, PINS_PREFIX "/%s", gpio_get_class_name( type, class ) );
            PinHandlerContext *context = malloc( sizeof( PinHandlerContext ) );
            context->server_context = server_context;
            context->type = type;
            context->class = class;
            httpd_uri_t uri_definition = {
                    .uri = uri,
                    .method = HTTP_PUT,
                    .handler = pins_put_config_handler,
                    .user_ctx = context
            };
            ESP_ERROR_CHECK( httpd_register_uri_handler( server, &uri_definition ) );
            snprintf( uri, sizeof uri, PINS_PREFIX "/%s/*", gpio_get_class_name( type, class ) );
            uri_definition.handler = pins_put_state_handler;
            uri_definition.uri = uri;
            ESP_ERROR_CHECK( httpd_register_uri_handler( server, &uri_definition ) );
            ESP_LOGI( LOG_TAG, "Registered PUT %s", uri );
        }
    }
}

void rest_register_gpio_handlers( httpd_handle_t server, http_server_context_t *server_context ) {
    rest_register_state_handler( server, server_context );
    rest_register_put_handlers( server, server_context );
    rest_register_get_handler( server, server_context );
}

