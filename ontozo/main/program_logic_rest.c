#include <esp_log.h>
#include "gpio_define.h"
#include "gpio_logic.h"
#include "program.h"
#include "program_logic.h"
#include "program_logic_rest.h"
#include "rest_util.h"

#define RUN_PREFIX "/run/"
#define RUN_URI RUN_PREFIX "*"

static const char *LOG_TAG = "program_logic";

static esp_err_t run_post_handler( httpd_req_t *req ) {
    esp_err_t result;
    int index;
    const char *uri = req->uri + strlen( RUN_PREFIX );

    ESP_LOGI( LOG_TAG, "%s %s", http_method_str( req->method ), req->uri );

    rest_allow_cors( req );
    if ( strcmp( uri, "stop" ) == 0 ) {
        program_logic_stop();
        rest_send_message_back( req, "stopped" );
    } else if ( strcmp( uri, "next/zone" ) == 0 ) {
        program_logic_move_to_next_zone();
        rest_send_message_back( req, "moved to next zone" );
    } else if ( strcmp( uri, "next/program" ) == 0 ) {
        program_logic_move_to_next_program();
        rest_send_message_back( req, "moved to next program" );
    } else if ( strncmp( uri, "start/", 6 /*strlen("start/")*/) == 0 ) {
        uri += 6;
        if ( ( result = rest_parse_index( uri, &index, true ) ) != ESP_OK ) {
            return rest_set_error_code( req, result, "Program index expected in URL" );
        }
        if ( index < 1 || index > program_get_count() ) {
            return rest_set_error_code( req, result, "Program index is wrong" );
        }
        program_logic_start( index - 1 );
        rest_send_message_back( req, "started" );
    } else {
        ESP_LOGW( LOG_TAG, "Unknown command in %s", req->uri );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "Unknown command" );
    }

    return ESP_OK;
}

static void add_program_json( cJSON *jsonPrograms, int programIndex, bool isActive, RunningProgramState *state ) {
    Program *program = program_get( programIndex );
    cJSON *jsonProgram = cJSON_CreateObject();
    cJSON_AddItemToArray( jsonPrograms, jsonProgram );
    cJSON_AddNumberToObject( jsonProgram, "index", programIndex + 1 );
    cJSON_AddStringToObject( jsonProgram, "name", program->name );
    PinData pin_data;
    cJSON *jsonZones = cJSON_AddArrayToObject( jsonProgram, "zones" );
    for ( int zone_index = 0; zone_index < program->zones_count; zone_index++ ) {
        cJSON *jsonZone = cJSON_CreateObject();
        cJSON_AddItemToArray( jsonZones, jsonZone );
        ProgramZone *zone = &program->zones[ zone_index ];
        gpio_get_pin_data( OUTPUTS, ZONES_CLASS, zone->zone_id, &pin_data );
        cJSON_AddNumberToObject( jsonZone, "duration", zone->duration_in_seconds );
        cJSON_AddNumberToObject( jsonZone, "index", zone_index + 1 );
        cJSON_AddStringToObject( jsonZone, "name", pin_data.name );
        if ( isActive && zone_index == ( *state ).zone_index ) {
            cJSON_AddBoolToObject( jsonZone, "running", true );
            cJSON_AddNumberToObject( jsonZone, "leftSeconds", ( *state ).zone_left_seconds );
        }
    }
}

static esp_err_t run_get_handler( httpd_req_t *req ) {
    ESP_LOGI( LOG_TAG, "%s %s", http_method_str( req->method ), req->uri );

    cJSON *jsonRoot = cJSON_CreateObject();
    cJSON_AddStringToObject( jsonRoot, "version", program_get_version() );

    RunningProgramState state;
    program_logic_get_state( &state );
    cJSON_AddBoolToObject( jsonRoot, "isProgramRunning", state.is_program_running );
    if ( state.is_program_running ) {
        cJSON *jsonPrograms = cJSON_AddArrayToObject( jsonRoot, "programs" );

        add_program_json( jsonPrograms, state.program_index, true, &state );
        for ( int queue_index = 0;; queue_index++ ) {
            int programIndex = program_logic_get_queued_program( queue_index );
            if ( programIndex < 0 ) {
                break;
            }
            add_program_json( jsonPrograms, programIndex, false, &state );
        }
    }
    rest_allow_cors( req );
    rest_add_time_json( jsonRoot );
    rest_send_json_back_and_delete( req, jsonRoot );
    return ESP_OK;
}

void rest_register_program_logic_handlers( httpd_handle_t server, http_server_context_t *server_context ) {
    httpd_uri_t run_post_uri = {
            .uri = RUN_URI,
            .method = HTTP_POST,
            .handler = run_post_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &run_post_uri ) );

    httpd_uri_t run_get_uri = {
            .uri = "/run",
            .method = HTTP_GET,
            .handler = run_get_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &run_get_uri ) );
}
