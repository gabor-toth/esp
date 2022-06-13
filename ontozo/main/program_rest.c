#include "lib/rest_util.h"
#include "program_rest.h"
#include "program_json.h"

//static const char *LOG_TAG = "program_rest";

#define PROGRAMS_PREFIX    "/programs/"
#define PROGRAMS_URI    PROGRAMS_PREFIX "*"

static esp_err_t programs_get_handler( httpd_req_t *req ) {
    char *json_out;

    programs_header_write_to_json( &json_out );
    rest_set_json_content_type( req );
    httpd_resp_sendstr( req, json_out );
    free((void *) json_out );
    return ESP_OK;
}

static esp_err_t program_get_handler( httpd_req_t *req ) {
    char *json_out;
    int index;
    esp_err_t result;

    if (( result = rest_parse_index( req->uri + strlen( PROGRAMS_PREFIX ), &index, true )) != ESP_OK ) {
        return rest_set_error_code( req, result, "Program index expected in URL" );
    }
    Program *program = program_get( index - 1 );
    if ( program == NULL) {
        return httpd_resp_send_err( req, HTTPD_404_NOT_FOUND, "Program not found" );
    }
    program_write_to_string( program, &json_out );
    rest_set_json_content_type( req );
    httpd_resp_sendstr( req, json_out );
    free((void *) json_out );
    return ESP_OK;
}

static void send_index_back( httpd_req_t *req, int index ) {
    char response[256];
    snprintf( response, sizeof response,
              "{ \"%s\": %d }", FIELD_INDEX, index );
    rest_set_json_content_type( req );
    httpd_resp_sendstr( req, response );
}

static esp_err_t program_put_post_handler( httpd_req_t *req, bool is_put ) {
    esp_err_t result;

    cJSON *root;
    result = rest_receive_json_body( req, (rest_server_context_t *) req->user_ctx, &root );
    if ( result != ESP_OK ) {
        return ESP_OK;
    }

    Program *program;
    program_read_from_json( root, &program );
    if ( !program->valid ) {
        return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Bad request, see log for more information" );
    }
    if ( !is_put ) {
        if ( program->index == 0 ) {
            return httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Needs an index for POST" );
        }
        program_change( program );
    } else {
        program_add( program );
    }

    send_index_back( req, program->index );

    return ESP_OK;
}

static esp_err_t program_put_handler( httpd_req_t *req ) {
    return program_put_post_handler( req, true );
}

static esp_err_t program_post_handler( httpd_req_t *req ) {
    return program_put_post_handler( req, false );
}

static esp_err_t program_delete_handler( httpd_req_t *req ) {
    esp_err_t result;
    int index;

    if (( result = rest_parse_index( req->uri + strlen( PROGRAMS_PREFIX ), &index, true )) != ESP_OK ) {
        return rest_set_error_code( req, result, "Program index expected in URL" );
    }
    if (( result = program_delete( index - 1 )) != ESP_OK ) {
        return rest_set_error_code( req, result, "Program not found" );
    }

    send_index_back( req, index );
    return ESP_OK;
}

void rest_register_programs_handlers( httpd_handle_t server, rest_server_context_t *rest_context ) {
    httpd_uri_t program_put_uri = {
            .uri = "/programs",
            .method = HTTP_PUT,
            .handler = program_put_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &program_put_uri );

    httpd_uri_t program_post_uri = {
            .uri = "/programs",
            .method = HTTP_POST,
            .handler = program_post_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &program_post_uri );

    httpd_uri_t program_delete_uri = {
            .uri = PROGRAMS_URI,
            .method = HTTP_DELETE,
            .handler = program_delete_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &program_delete_uri );

    httpd_uri_t program_get_uri = {
            .uri = "/programs/*",
            .method = HTTP_GET,
            .handler = program_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &program_get_uri );

    httpd_uri_t programs_get_uri = {
            .uri = "/programs",
            .method = HTTP_GET,
            .handler = programs_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &programs_get_uri );
}
