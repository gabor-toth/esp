#include "config_version.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_main.h"
#include "program.h"
#include "program_json.h"
#include <string.h>

static const char* LOG_TAG = "program";

#define PROGRAM_INITIAL_COUNT   8
#define NVS_NAMESPACE_PROGRAM "program"
#define NVS_NAME_PROGRAM_VERSION  "version"
#define NVS_NAME_PROGRAM_NEXT_ID  "nextId"
#define NVS_NAME_PROGRAM_PREFIX  "#"
#define NVS_NAME_PROGRAM_MASK  NVS_NAME_PROGRAM_PREFIX"%d"

static uint16_t program_next_id = 1;
static int program_count = 0;
static int program_max_count = PROGRAM_INITIAL_COUNT;
static Program** programs;
static ConfigVersion version;

static void get_nvs_key( uint id, char* name_buffer, int name_buffer_size ) {
    snprintf( name_buffer, name_buffer_size, NVS_NAME_PROGRAM_MASK, id );
}

Program* program_constructor() {
    Program* program = calloc( 1, sizeof( Program ) );
    return program;
}

void program_destructor( Program* program ) {
    if ( program == NULL ) {
        return;
    }
    if ( program->name ) {
        free( program->name );
    }
    if ( program->start_times ) {
        free( program->start_times );
    }
    if ( program->zones ) {
        free( program->zones );
    }
    free( program );
}

static void write_program_to_nvs( Program* program ) {
    uint32_t nvs_handle = nvs_open_storage( NVS_NAMESPACE_PROGRAM );
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
        return;
    }

    char nvs_key[ 256 ];
    get_nvs_key( program->id, nvs_key, sizeof nvs_key );
    char* json_out;
    program_write_to_string( program, &json_out );
    nvs_write_string( nvs_handle, nvs_key, json_out );
    free( json_out );
    config_version_set_and_write( nvs_handle, &version );
    nvs_write_u16( nvs_handle, NVS_NAME_PROGRAM_NEXT_ID, program_next_id );
    nvs_close_storage( nvs_handle );
}

static void program_add_to_list( Program* program ) {
    if ( program_count == program_max_count ) {
        program_max_count += PROGRAM_INITIAL_COUNT;
        programs = reallocarray( program, program_max_count, sizeof( Program* ) );
    }
    programs[ program_count++ ] = program;
}

static int get_index_by_id( int id ) {
    for ( int i = 0; i < program_count; i++ ) {
        if ( programs[ i ]->id == id ) {
            return i;
        }
    }
    ESP_LOGW( LOG_TAG, "Program id %d not found", id );
    return -1;
}

esp_err_t program_add( Program* program ) {
    program->id = program_next_id++;
    program_add_to_list( program );
    write_program_to_nvs( program );
    return ESP_OK;
}

esp_err_t program_change( Program* program ) {
    int index = get_index_by_id( program->id );
    if ( index < 0 ) {
        return ESP_ERR_NOT_FOUND;
    }
    program_destructor( programs[ index ] );
    programs[ index ] = program;

    write_program_to_nvs( program );
    return ESP_OK;
}

esp_err_t program_delete( int id ) {
    int index = get_index_by_id( id );
    if ( index < 0 ) {
        return ESP_ERR_NOT_FOUND;
    }
    program_destructor( programs[ index ] );
    memcpy( &programs[ index ], &programs[ index + 1 ], ( program_count - index ) * sizeof( Program* ) );
    program_count--;

    uint32_t nvs_handle = nvs_open_storage( NVS_NAMESPACE_PROGRAM );
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
    } else {
        char nvs_key[ 256 ];
        get_nvs_key( id, nvs_key, sizeof nvs_key );
        nvs_delete( nvs_handle, nvs_key );
        config_version_set_and_write( nvs_handle, &version );
        nvs_close_storage( nvs_handle );
    }
    return ESP_OK;
}

int program_get_count() {
    return program_count;
}

Program* program_get( int index ) {
    if ( index < 0 || index >= program_count ) {
        ESP_LOGW( LOG_TAG, "Program index %d is out of range 1..%d", index, program_count );
        return NULL;
    }
    return programs[ index ];
}

const char* program_get_version() {
    return version.string;
}

void program_init() {
    uint32_t nvs_handle = nvs_open_storage( NVS_NAMESPACE_PROGRAM );
    if ( nvs_handle == 0 ) {
        ESP_LOGW( LOG_TAG, "Unable to open NVS" );
        return;
    }

    config_version_init( &version, NVS_NAME_PROGRAM_VERSION );

    if ( nvs_get_u16( nvs_handle, NVS_NAME_PROGRAM_NEXT_ID, &program_next_id ) != ESP_OK ) {
        program_next_id = 1;
    }
    ESP_LOGI( LOG_TAG, "Next program id is %d", program_next_id );

    config_version_read( nvs_handle, &version );

    programs = malloc( sizeof( Program* ) * program_max_count );
    program_max_count = PROGRAM_INITIAL_COUNT;

    nvs_iterator_t it = NULL;
    // esp_err_t res = nvs_entry_find( NVS_STORAGE_NAMESPACE, NVS_NAMESPACE_PROGRAM, NVS_TYPE_ANY, &it );
    esp_err_t res = nvs_entry_find_in_handle( nvs_handle, NVS_TYPE_ANY, &it );
    while ( res == ESP_OK ) {
        nvs_entry_info_t info;
        nvs_entry_info( it, &info );
        //printf( "namespace %-15s key %-15s type %d\n", info.namespace_name, info.key, info.type );
        if ( strncmp( info.key, NVS_NAME_PROGRAM_PREFIX, strlen( NVS_NAME_PROGRAM_PREFIX ) ) == 0 ) {
            const char* program_id = info.key + strlen( NVS_NAME_PROGRAM_PREFIX );
            Program* program = NULL;
            char* program_as_json_string = nvs_read_string( nvs_handle, info.key );
            if ( program_as_json_string == NULL ) {
                ESP_LOGW( LOG_TAG, "Unable to read program %s", program_id );
            } else {
                program_read_from_string( program_as_json_string, &program );
                if ( !program->valid ) {
                    ESP_LOGE( LOG_TAG, "Unable to parse program %s", program_id );
                } else {
                    program_add_to_list( program );
                }
            }
        }
        res = nvs_entry_next( &it );
    }
    nvs_release_iterator( it );

    nvs_close_storage( nvs_handle );
}
