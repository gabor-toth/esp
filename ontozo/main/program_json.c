#include "esp_log.h"
#include "cJSON.h"
#include "esp_err.h"
#include <string.h>
#include <stdio.h>

#include "program_json.h"
#include "program.h"

static char *const LOG_TAG = "program_json";

static char *const FIELD_NAME = "name";
static char *const FIELD_START_TIMES = "startTimes";
static char *const FIELD_ZONES = "zones";
static char *const FIELD_ZONE_ID = "zoneId";
static char *const FIELD_DURATION = "duration";
static char *const FIELD_DAYS = "days";
const char *const FIELD_TYPE = "type";
const char *const VALUE_TYPE_ON = "on";
const char *const VALUE_TYPE_INTERVAL = "interval";
const char *const VALUE_TYPE_UNUSED = "unused";
const char *const FIELD_ON_DAYS = "onDays";
const char *const FIELD_INTERVAL_DAYS = "intervalDays";
const char *const FIELD_INTERVAL_STARTS_ON = "intervalStartsOn";
const char *const FIELD_INTERVAL_START_RESET = "intervalStartReset";

const char *const VALUE_DAY_NAMES[] = { "", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
#define VALUE_DAY_NAMES_COUNT (sizeof VALUE_DAY_NAMES / sizeof VALUE_DAY_NAMES[ 0 ])

/*
    {
        "name": "name",
        "startTimes": [
            "08:20",
            "14:40"
        ],
        "zones": [
            {"zoneId": 1, "duration": 600},
            {"zoneId": 2, "duration": 300},
            {"zoneId": 1, "duration": 60}
        ],
        "days": {
            "type": "on|interval",
            "onDays": ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"],
            "intervalDays": 2,
            "intervalStartsOn": "Tue",
            "intervalStartReset": false
        }
    }
 */

static esp_err_t read_name( const cJSON *root, Program *program ) {
    cJSON *name_element = cJSON_GetObjectItem( root, FIELD_NAME );
    if ( name_element == NULL) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_NAME );
        return ESP_ERR_INVALID_ARG;
    }
    program->name = name_element->valuestring;
    return ESP_OK;
}

static esp_err_t read_start_times( const cJSON *root, Program *program ) {
    /*
        "startTimes": [
            0820,
            1440
        ],
     */
    cJSON *start_times_element = cJSON_GetObjectItem( root, FIELD_START_TIMES );
    if ( start_times_element == NULL) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_START_TIMES );
        return ESP_ERR_INVALID_ARG;
    }
    int count = cJSON_GetArraySize( start_times_element );
    program->start_times_count = count;
    program->start_times = malloc( sizeof program->start_times[ 0 ] * count );
    for ( int i = 0; i < count; i++ ) {
        cJSON *start_time_element = cJSON_GetArrayItem( start_times_element, i );
        char *start_time_string = start_time_element->valuestring;
        int hour, minute;
        if ( sscanf( start_time_string, "%d:%d", &hour, &minute ) == 2 ) {
            program->start_times[ i ] = hour * 100 + minute;
        }
    }
    return ESP_OK;
}

esp_err_t read_zones( const cJSON *root, Program *program ) {
    /*
        "zones": [
            {"zoneId": 1, "duration": 600},
            {"zoneId": 2, "duration": 300},
            {"zoneId": 1, "duration": 60},
        ],
     */
    cJSON *zones_element = cJSON_GetObjectItem( root, FIELD_ZONES );
    if ( zones_element == NULL) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_ZONES );
        return ESP_ERR_INVALID_ARG;
    }
    int count = cJSON_GetArraySize( zones_element );
    program->zones_count = count;
    program->zones = malloc( sizeof program->zones[ 0 ] * count );
    for ( int i = 0; i < count; i++ ) {
        cJSON *zone_element = cJSON_GetArrayItem( zones_element, i );
        cJSON *zone_id_element = cJSON_GetObjectItem( zone_element, FIELD_ZONE_ID );
        if ( zone_id_element ) {
            program->zones[ i ].zone_id = zone_id_element->valueint;
        } else {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_ZONE_ID );
        }
        cJSON *duration_element = cJSON_GetObjectItem( zone_element, FIELD_DURATION );
        if ( duration_element ) {
            program->zones[ i ].duration_in_seconds = duration_element->valueint;
        } else {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_DURATION );
        }
    }
    return ESP_OK;
}

static int get_day_index( const char *day_name ) {
    for ( int i = 1; i < VALUE_DAY_NAMES_COUNT; i++ ) {
        if ( strcmp( day_name, VALUE_DAY_NAMES[ i ] ) == 0 ) {
            return i;
        }
    }
    ESP_LOGW( LOG_TAG, "unknown day %s", day_name );
    return 0;
}

esp_err_t read_days( const cJSON *root, Program *program ) {
    /*
        "days": {
            "type": "on|interval",
            "onDays": "Mon,Tue,Wed,Thu,Fri,Sat,Sun"
            "intervalDays": 2,
            "intervalStartsOn": "Tue",
            "intervalStartReset": false,
        },
     */
    cJSON *days_element = cJSON_GetObjectItem( root, FIELD_DAYS );
    if ( days_element == NULL) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_DAYS );
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *type_element = cJSON_GetObjectItem( days_element, FIELD_TYPE );
    if ( !type_element ) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_TYPE );
        return ESP_ERR_INVALID_ARG;
    }
    char *type_as_string = type_element->valuestring;
    if ( strcmp( VALUE_TYPE_ON, type_as_string ) == 0 ) {
        cJSON *on_days_element = cJSON_GetObjectItem( days_element, FIELD_ON_DAYS );
        if ( !on_days_element ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_ON_DAYS );
            return ESP_ERR_INVALID_ARG;
        }
        int day_count = cJSON_GetArraySize( on_days_element );
        for ( int d = 0; d < day_count; d++ ) {
            char *day_name = cJSON_GetArrayItem( on_days_element, d )->valuestring;
            int day_index = get_day_index( day_name );
            if ( day_index != 0 ) {
                program->days.on_days |= 1 << day_index;
            }
        }
        program->days.type = on;
    } else if ( strcmp( VALUE_TYPE_INTERVAL, type_as_string ) == 0 ) {
        cJSON *interval_days_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_DAYS );
        if ( !interval_days_element ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_INTERVAL_DAYS );
            return ESP_ERR_INVALID_ARG;
        }
        cJSON *interval_start_on_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_STARTS_ON );
        if ( !interval_start_on_element ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_INTERVAL_STARTS_ON );
            return ESP_ERR_INVALID_ARG;
        }
        cJSON *interval_start_reset_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_START_RESET );
        if ( interval_start_reset_element ) {
            program->days.interval_start_reset = interval_start_reset_element->valueint;
        } else {
            ESP_LOGI( LOG_TAG, "has no %s", FIELD_INTERVAL_START_RESET );
        }
        program->days.type = interval;
        program->days.interval_days = interval_days_element->valueint;
        program->days.interval_start_day = get_day_index( interval_start_on_element->valuestring );
    } else {
        ESP_LOGW( LOG_TAG, "wrong %s %s", FIELD_TYPE, type_as_string );
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t program_read_from_json( cJSON *root, Program **program_out ) {
    *program_out = NULL;
    Program *program = program_constructor();

    esp_err_t result;
    if (( result = read_name( root, program )) != ESP_OK ||
        ( result = read_days( root, program )) != ESP_OK ||
        ( result = read_start_times( root, program )) != ESP_OK ||
        ( result = read_zones( root, program )) != ESP_OK ) {
        program_destructor( program );
        return result;
    }

    *program_out = program;
    return ESP_OK;
}

void write_name( cJSON *json, Program *program ) {
    cJSON_AddStringToObject( json, FIELD_NAME, program->name );
}

void write_days( cJSON *json, Program *program ) {
    /*
        "days": {
            "type": "on|interval",
            "onDays": ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"],
            "intervalDays": 2,
            "intervalStartsOn": "Tue",
            "intervalStartReset": false,
        },
     */
    cJSON *days = cJSON_AddObjectToObject( json, FIELD_DAYS );
    if ( program->days.type == on ) {
        cJSON_AddStringToObject( days, FIELD_TYPE, VALUE_TYPE_ON );
        cJSON *days_array = cJSON_AddArrayToObject( days, FIELD_DAYS );
        for ( int day_index = 1; day_index <= VALUE_DAY_NAMES_COUNT; day_index++ ) {
            if ( program->days.on_days & ( 1 << day_index )) {
                cJSON_AddItemToArray( days_array, cJSON_CreateString( VALUE_DAY_NAMES[ day_index ] ));
            }
        }
    } else if ( program->days.type == interval ) {
        cJSON_AddStringToObject( days, FIELD_TYPE, VALUE_TYPE_INTERVAL );
        cJSON_AddNumberToObject( days, FIELD_INTERVAL_START_RESET, program->days.interval_days );
        cJSON_AddStringToObject( days, FIELD_INTERVAL_STARTS_ON, VALUE_DAY_NAMES[ program->days.interval_start_day ] );
    } else {
        cJSON_AddStringToObject( days, FIELD_TYPE, VALUE_TYPE_UNUSED );
    }
}

void write_start_times( cJSON *json, Program *program ) {
    /*
        "startTimes": [
            0820,
            1440
        ],
     */

    char format_buf[32];

    cJSON *array = cJSON_AddArrayToObject( json, FIELD_START_TIMES );
    for ( int i = 0; i < program->start_times_count; i++ ) {
        int start_time = program->start_times[ i ];
        snprintf( format_buf, sizeof format_buf, "%02d:%02d", start_time / 100, start_time % 100 );
        cJSON_AddItemToArray( array, cJSON_CreateString( format_buf ));
    }
}

void write_zones( cJSON *json, Program *program ) {
    /*
        "zones": [
            {"zoneId": 1, "duration": 600},
            {"zoneId": 2, "duration": 300},
            {"zoneId": 1, "duration": 60},
        ],
     */
    cJSON *array = cJSON_AddArrayToObject( json, FIELD_ZONES );
    for ( int i = 0; i < program->zones_count; i++ ) {
        ProgramZone *zone = program->zones + i;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddItemToArray( array, item );
        cJSON_AddNumberToObject( item, FIELD_ZONE_ID, zone->zone_id );
        cJSON_AddNumberToObject( item, FIELD_DURATION, zone->duration_in_seconds );
    }
}

esp_err_t program_write_to_json( Program *program, char **json_out ) {
    cJSON *root = cJSON_CreateObject();

    write_name( root, program );
    write_days( root, program );
    write_start_times( root, program );
    write_zones( root, program );

    *json_out = cJSON_Print( root );
    cJSON_Delete( root );
    return ESP_OK;
}

