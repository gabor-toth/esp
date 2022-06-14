#include <esp_log.h>
#include "lib/rest_util.h"
#include "program_logic_rest.h"
#include "program_logic.h"

#define RUN_PREFIX "/run/"
#define RUN_URI RUN_PREFIX "*"

static const char *LOG_TAG = "program_logic";

static esp_err_t run_post_handler( httpd_req_t *req ) {
    esp_err_t result;
    int index;
    const char *uri = req->uri + strlen( RUN_PREFIX );

    if ( strcmp( uri, "stop" ) == 0 ) {
        program_logic_stop();
        rest_send_message_back( req, "stopped" );
    } else if ( strcmp( uri, "next" ) == 0 ) {
        program_logic_move_to_next_zone();
        rest_send_message_back( req, "moved to next" );
    } else if ( strncmp( uri, "start/", 6 /*strlen("start/")*/) == 0 ) {
        uri += 6;
        if (( result = rest_parse_index( uri, &index, true )) != ESP_OK ) {
            return rest_set_error_code( req, result, "Program index expected in URL" );
        }
        program_logic_start( index - 1 );
        rest_send_message_back( req, "started" );
    } else {
        ESP_LOGW( LOG_TAG, "Unknown command in %s", req->uri );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "Unknown command" );
    }

    return ESP_OK;
}

static esp_err_t run_get_handler( httpd_req_t *req ) {
    cJSON *root = cJSON_CreateObject();

    RunningProgramState state;
    program_logic_get_state( &state );
    cJSON_AddBoolToObject( root, "isProgramRunning", state.is_program_running );
    if ( state.is_program_running ) {
        cJSON_AddNumberToObject( root, "programIndex", state.program_index + 1 );
        cJSON_AddNumberToObject( root, "zoneIndex", state.zone_index + 1 );
        cJSON_AddNumberToObject( root, "zonesCount", state.zones_count );
        cJSON_AddNumberToObject( root, "zoneLeftSeconds", state.zone_left_seconds );
    }
    rest_send_json_back( req, root );
    return ESP_OK;
}

void rest_register_program_logic_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t run_post_uri = {
            .uri = RUN_URI,
            .method = HTTP_POST,
            .handler = run_post_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &run_post_uri );

    httpd_uri_t run_get_uri = {
            .uri = "/run",
            .method = HTTP_GET,
            .handler = run_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &run_get_uri );
}
