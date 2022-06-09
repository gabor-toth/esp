#include "program_rest.h"
#include "program_json.h"

#define PROGRAMS_PREFIX    "/programs/"
#define PROGRAMS_URI    PROGRAMS_PREFIX "*"

static esp_err_t programs_get_handler( httpd_req_t *req ) {
    char *json_out;
    esp_err_t result = programs_header_write_to_json( &json_out );
    if ( result != ESP_OK ) {
        return result;
    }
    set_json_content_type( req );
    httpd_resp_sendstr( req, json_out );
    free((void *) json_out );
    return ESP_OK;
}

static esp_err_t program_put_post_handler( httpd_req_t *req, bool is_put ) {

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
    if ( !is_put ) {
        if ( program->index == 0 ) {
            httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Needs an index for POST" );
            return ESP_FAIL;
        }
        program_change( program );
    } else {
        program_add( program );
    }

    char response[256];
    snprintf( response,
              sizeof response, "{ \"id\": %d }", program->index );
    set_json_content_type( req );
    httpd_resp_sendstr( req, response );

    return ESP_OK;
}

static esp_err_t program_put_handler( httpd_req_t *req ) {
    return program_put_post_handler( req, true );
}

static esp_err_t program_post_handler( httpd_req_t *req ) {
    return program_put_post_handler( req, false );
}

static esp_err_t program_delete_handler( httpd_req_t *req ) {
    printf( "%s\n", req->uri );
    return ESP_ERR_INVALID_STATE; //program_put_post_handler( req, false );
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

    httpd_uri_t programs_get_uri = {
            .uri = "/programs*",
            .method = HTTP_GET,
            .handler = programs_get_handler,
            .user_ctx = rest_context
    };
    httpd_register_uri_handler( server, &programs_get_uri );
}
