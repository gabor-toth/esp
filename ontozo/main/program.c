#include "esp_log.h"
#include "program.h"
#include "program_json.h"
#include "lib/nvs_main.h"
#include "nvs.h"
#include <string.h>

static const char *LOG_TAG = "program";

#define PROGRAM_INITIAL_COUNT   8
#define NVS_NAME_PREFIX  "irr.prg."
#define NVS_NAME_PROGRAM_COUNT  NVS_NAME_PREFIX "cnt"
#define NVS_NAME_PROGRAM_MASK  NVS_NAME_PREFIX "%d"

static int program_count = 0;
static int program_max_count = PROGRAM_INITIAL_COUNT;
static Program **programs;

static void get_nvs_key( uint index, char *name_buffer, int name_buffer_size ) {
    snprintf( name_buffer, name_buffer_size, NVS_NAME_PROGRAM_MASK, index );
}

Program *program_constructor() {
    Program *program = calloc( 1, sizeof( Program ));
    return program;
}

void program_destructor( Program *program ) {
    if ( program == NULL) {
        return;
    }
    free( program->name );
    free( program->start_times );
    free( program->zones );
    free( program );
}

static void write_program_to_nvs( Program *program ) {
    uint32_t nvs_handle = nvs_open_storage();
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
        return;
    }

    char nvs_key[256];
    get_nvs_key( program->index - 1, nvs_key, sizeof nvs_key );
    char *json_out;
    program_write_to_string( program, &json_out );
    nvs_write_string( nvs_handle, nvs_key, json_out );
    free( json_out );
    nvs_set_u32( nvs_handle, NVS_NAME_PROGRAM_COUNT, program_count );
    nvs_close_storage( nvs_handle );
}

static void program_add_to_list( Program *program ) {
    if ( program_count == program_max_count ) {
        program_max_count += PROGRAM_INITIAL_COUNT;
        programs = reallocarray( program, sizeof( Program * ), program_max_count );
    }
    programs[ program_count++ ] = program;
}

static void do_program_add( Program *program, bool from_init ) {
    program_add_to_list( program );
    if ( program != NULL) {
        program->index = program_count;
        if ( !from_init ) {
            write_program_to_nvs( program );
        }
    }
}

void program_add( Program *program ) {
    do_program_add( program, false );
}

void program_change( Program *program ) {
    int index = program->index - 1;
    if ( index < 0 || index >= program_count ) {
        return;
    }
    program_destructor( programs[ index ] );
    programs[ index ] = program;

    write_program_to_nvs( program );
}

esp_err_t program_delete( int index ) {
    if ( index < 0 || index >= program_count ) {
        ESP_LOGW( LOG_TAG, "Program index %d is out of range 1..%d", index, program_count );
        return ESP_ERR_NOT_FOUND;
    }
    program_destructor( programs[ index ] );

    uint32_t nvs_handle = nvs_open_storage();
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
    } else {
        char nvs_key[256];
        for ( int i = index; i < program_count - 1; i++ ) {
            programs[ i ] = programs[ i + 1 ];
            get_nvs_key( i + 1, nvs_key, sizeof nvs_key );
            char *program_as_json_string = nvs_read_string( nvs_handle, nvs_key );
            if ( program_as_json_string != NULL) {
                get_nvs_key( i, nvs_key, sizeof nvs_key );
                nvs_write_string( nvs_handle, nvs_key, program_as_json_string );
                free( program_as_json_string );
            }
        }
        get_nvs_key( program_count - 1, nvs_key, sizeof nvs_key );
        nvs_delete( nvs_handle, nvs_key );
        nvs_close_storage( nvs_handle );
    }
    program_count--;
    return ESP_OK;
}

int program_get_count() {
    return program_count;
}

Program *program_get( int index ) {
    if ( index < 0 || index >= program_count ) {
        ESP_LOGW( LOG_TAG, "Program index %d is out of range 1..%d", index, program_count );
        return NULL;
    }
    return programs[ index ];
}

void program_init() {
    programs = malloc( sizeof( Program * ) * program_max_count );

    uint32_t nvs_handle = nvs_open_storage();
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
    }
    uint32_t count;
    char nvs_key[256];

    if ( nvs_get_u32( nvs_handle, NVS_NAME_PROGRAM_COUNT, &count ) == ESP_OK ) {
        ESP_LOGI( LOG_TAG, "Reading %d programs", count );
        for ( uint i = 0; i < count; i++ ) {
            Program *program = NULL;
            get_nvs_key( i, nvs_key, sizeof nvs_key );
            char *program_as_json_string = nvs_read_string( nvs_handle, nvs_key );
            if ( program_as_json_string == NULL) {
                ESP_LOGE( LOG_TAG, "Unable to read program %d", i );
                do_program_add(NULL, true );
            } else {
                program_read_from_string( program_as_json_string, &program );
                if ( !program->valid ) {
                    ESP_LOGE( LOG_TAG, "Unable to parse program %d", i );
                }
                do_program_add( program, true );
            }
        }
    } else {
        ESP_LOGI( LOG_TAG, "No programs stored" );
    }
    nvs_close_storage( nvs_handle );
}

