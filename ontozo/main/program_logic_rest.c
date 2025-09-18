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

    bool found = false;
    rest_allow_cors( req );
    if ( strcmp( uri, "stop" ) == 0 ) {
        program_logic_stop_all();
        rest_send_message_back( req, "stopped" );
        found = true;
    } else if ( strcmp( uri, "next/zone" ) == 0 ) {
        // TODO pass in current ids
        program_logic_move_to_next_zone( 0, 0 );
        rest_send_message_back( req, "moved to next zone" );
        found = true;
    } else if ( strcmp( uri, "next/program" ) == 0 ) {
        // TODO pass in current ids
        program_logic_move_to_next_program( 0 );
        rest_send_message_back( req, "moved to next program" );
        found = true;
    } else if ( strncmp( uri, "start/", 6 /*strlen("start/")*/) == 0 ) {
        uri += 6;
        if ( ( result = rest_parse_index( &uri, &index, true ) ) != ESP_OK ) {
            return rest_set_error_code( req, result, "Program index expected in URL" );
        }
        if ( index < 1 || index > program_get_count() ) {
            return rest_set_error_code( req, result, "Program index is wrong" );
        }
        program_logic_start( index - 1 );
        rest_send_message_back( req, "started" );
        found = true;
    } else if ( strncmp( uri, "queued/", 7 ) == 0 ) {
        uri += 7;
        int program_id;
        if ( ( result = rest_parse_index( &uri, &program_id, true ) ) != ESP_OK ) {
            ESP_LOGW( LOG_TAG, "Program index expected in URL %s %s at %s", http_method_str( req->method ), req->uri,
                      uri );
            return rest_set_error_code( req, result, "Program index expected in URL" );
        }
        if ( req->method == HTTP_DELETE ) {
            program_logic_cancel_scheduled_program( program_id );
            found = true;
        } else if ( req->method == HTTP_POST ) {
            uri += 1;
            int zone_index;
            if ( ( result = rest_parse_index( &uri, &zone_index, true ) ) != ESP_OK ) {
                ESP_LOGW( LOG_TAG, "Zone index expected in URL %s %s at %s", http_method_str( req->method ), req->uri,
                          uri );
                return rest_set_error_code( req, result, "Zone index expected in URL" );
            }
            program_logic_toggle_scheduled_zone( program_id, zone_index );
            found = true;
        }
    }
    if ( !found ) {
        ESP_LOGW( LOG_TAG, "Unknown command in %s %s at %s", http_method_str( req->method ), req->uri, uri );
        return rest_set_error_code( req, ESP_ERR_INVALID_ARG, "Unknown command" );
    }

    return ESP_OK;
}

static void add_program_json( cJSON *jsonPrograms, int programIndex, QueuedProgramState *queued_state,
                              RunningProgramState *state ) {
    Program *program = program_get( programIndex );
    cJSON *jsonProgram = cJSON_CreateObject();
    cJSON_AddItemToArray( jsonPrograms, jsonProgram );
    cJSON_AddNumberToObject( jsonProgram, "id", (double) queued_state->program_id );
    cJSON_AddNumberToObject( jsonProgram, "index", programIndex + 1 );
    cJSON_AddStringToObject( jsonProgram, "name", program->name );
    PinData pin_data;
    cJSON *jsonZones = cJSON_AddArrayToObject( jsonProgram, "zones" );
    for ( int zone_index = 0; zone_index < program->zones_count; zone_index++ ) {
        cJSON *jsonZone = cJSON_CreateObject();
        cJSON_AddItemToArray( jsonZones, jsonZone );
        ProgramZone *zone = &program->zones[ zone_index ];
        gpio_get_pin_data( OUTPUTS, ZONES_CLASS, zone->zone_id, &pin_data );
        if ( zone_index < queued_state->zones_count ) {
            cJSON_AddBoolToObject( jsonZone, "enabled", !queued_state->zones_disabled[ zone_index ] );
        }
        cJSON_AddNumberToObject( jsonZone, "duration", zone->duration_in_seconds );
        cJSON_AddNumberToObject( jsonZone, "index", zone_index + 1 );
        cJSON_AddStringToObject( jsonZone, "name", pin_data.name );
        if ( state != NULL && zone_index == state->zone_index ) {
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
    QueuedProgramState *queued_programs;
    int queued_programs_count = program_logic_get_queued_programs( &state, &queued_programs );
    cJSON_AddBoolToObject( jsonRoot, "isProgramRunning", state.is_program_running );
    if ( queued_programs_count > 0 ) {
        cJSON *jsonPrograms = cJSON_AddArrayToObject( jsonRoot, "programs" );
        for ( int queue_index = 0; queue_index < queued_programs_count; queue_index++ ) {
            int programIndex = queued_programs[ queue_index ].program_index;
            add_program_json( jsonPrograms, programIndex,
                              queued_programs + queue_index,
                              queue_index == 0 ? &state : NULL );
        }
    }
    if ( queued_programs != NULL ) {
        free( queued_programs );
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

    httpd_uri_t run_delete_uri = {
            .uri = RUN_URI,
            .method = HTTP_DELETE,
            .handler = run_post_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &run_delete_uri ) );

    httpd_uri_t run_get_uri = {
            .uri = "/run",
            .method = HTTP_GET,
            .handler = run_get_handler,
            .user_ctx = server_context
    };
    ESP_ERROR_CHECK( httpd_register_uri_handler( server, &run_get_uri ) );
}
