#include "esp_log.h"
#include "cJSON.h"
#include "esp_err.h"
#include <string.h>
#include <stdio.h>

#include "lib/gpio_define.h"
#include "gpio_logic.h"
#include "program_json.h"
#include "program.h"

static char *const LOG_TAG = "program_json";

char *const FIELD_INDEX = "index";
static char *const FIELD_ENABLED = "enabled";
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
        "index": 3,
        "enabled": true,
        "name": "name",
    }
 */

static void read_head( const cJSON *root, Program *program ) {
    {
        cJSON *name_element = cJSON_GetObjectItem( root, FIELD_NAME );
        if ( name_element == NULL) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_NAME );
            program->valid = false;
        } else {
            program->name = strdup( name_element->valuestring );
        }
    }
    {
        cJSON *index_element = cJSON_GetObjectItem( root, FIELD_INDEX );
        if ( index_element != NULL) {
            program->index = index_element->valueint;
        }
    }
    {
        cJSON *enabled_element = cJSON_GetObjectItem( root, FIELD_ENABLED );
        if ( enabled_element != NULL) {
            program->enabled = enabled_element->valueint;
        }
    }
}

static void read_start_times( const cJSON *root, Program *program ) {
    /*
        "startTimes": [
            0820,
            1440
        ],
     */
    cJSON *start_times_element = cJSON_GetObjectItem( root, FIELD_START_TIMES );
    if ( start_times_element == NULL) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_START_TIMES );
        program->valid = false;
        return;
    }
    int count = cJSON_GetArraySize( start_times_element );
    program->start_times_count = count;
    program->start_times = malloc( sizeof program->start_times[ 0 ] * count );
    for ( int i = 0; i < count; i++ ) {
        cJSON *start_time_element = cJSON_GetArrayItem( start_times_element, i );
        char *start_time_string = start_time_element->valuestring;
        int hour, minute;
        if ( sscanf( start_time_string, "%d:%d", &hour, &minute ) == 2 ) {
            program->start_times[ i ] = PROGRAM_START_TIME( hour, minute );
        }
    }
}

void read_zones( const cJSON *root, Program *program ) {
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
        program->valid = false;
        return;
    }
    int count = cJSON_GetArraySize( zones_element );
    program->zones_count = count;
    program->zones = malloc( sizeof program->zones[ 0 ] * count );
    for ( int i = 0; i < count; i++ ) {
        cJSON *zone_element = cJSON_GetArrayItem( zones_element, i );
        cJSON *zone_id_element = cJSON_GetObjectItem( zone_element, FIELD_ZONE_ID );
        if ( zone_id_element ) {
            int zone_id = zone_id_element->valueint - 1;
            if ( !gpio_is_valid_index( OUTPUTS, ZONES_CLASS, zone_id )) {
                program->valid = false;
            } else {
                program->zones[ i ].zone_id = zone_id;
            }
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

void read_days( const cJSON *root, Program *program ) {
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
        program->valid = false;
        return;
    }
    cJSON *type_element = cJSON_GetObjectItem( days_element, FIELD_TYPE );
    program->days.type = unused;
    if ( !type_element ) {
        ESP_LOGW( LOG_TAG, "has no %s", FIELD_TYPE );
        program->valid = false;
    } else {
        char *type_as_string = type_element->valuestring;
        if ( strcmp( VALUE_TYPE_ON, type_as_string ) == 0 ) {
            program->days.type = on;
        } else if ( strcmp( VALUE_TYPE_INTERVAL, type_as_string ) == 0 ) {
            program->days.type = interval;
        } else {
            ESP_LOGW( LOG_TAG, "wrong %s %s", FIELD_TYPE, type_as_string );
            program->valid = false;
        }
    }
    cJSON *on_days_element = cJSON_GetObjectItem( days_element, FIELD_ON_DAYS );

    if ( !on_days_element ) {
        if ( program->days.type == on ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_ON_DAYS );
            program->valid = false;
        }
    } else {
        int day_count = cJSON_GetArraySize( on_days_element );
        for ( int d = 0; d < day_count; d++ ) {
            char *day_name = cJSON_GetArrayItem( on_days_element, d )->valuestring;
            int day_index = get_day_index( day_name );
            if ( day_index != 0 ) {
                program->days.on_days |= 1 << day_index;
            }
        }
    }

    cJSON *interval_days_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_DAYS );
    if ( !interval_days_element ) {
        if ( program->days.type == interval ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_INTERVAL_DAYS );
            program->valid = false;
        }
    } else {
        program->days.interval_days = interval_days_element->valueint;
    }

    cJSON *interval_start_on_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_STARTS_ON );
    if ( !interval_start_on_element ) {
        if ( program->days.type == interval ) {
            ESP_LOGW( LOG_TAG, "has no %s", FIELD_INTERVAL_STARTS_ON );
            program->valid = false;
        }
    } else {
        program->days.interval_start_day = get_day_index( interval_start_on_element->valuestring );
    }

    cJSON *interval_start_reset_element = cJSON_GetObjectItem( days_element, FIELD_INTERVAL_START_RESET );
    if ( interval_start_reset_element ) {
        program->days.interval_start_reset = interval_start_reset_element->valueint;
    }
}


void program_read_from_string( const char *json_string, Program **program_out ) {
    *program_out = NULL;
    cJSON *root = cJSON_Parse( json_string );
    if ( root == NULL) {
        return;
    }
    program_read_from_json( root, program_out );
    cJSON_Delete( root );
}

void program_read_from_json( cJSON *root, Program **program_out ) {
    *program_out = NULL;
    Program *program = program_constructor();
    program->valid = true;

    read_head( root, program );
    read_days( root, program );
    read_start_times( root, program );
    read_zones( root, program );
    // program_destructor( program );

    *program_out = program;
}

void write_head( cJSON *json, Program *program ) {
    cJSON_AddNumberToObject( json, FIELD_INDEX, program->index );
    cJSON_AddStringToObject( json, FIELD_NAME, program->name );
    cJSON_AddBoolToObject( json, FIELD_ENABLED, program->enabled );
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
    const char *type_as_string = program->days.type == on ? VALUE_TYPE_ON :
                                 program->days.type == interval ? VALUE_TYPE_INTERVAL :
                                 VALUE_TYPE_UNUSED;
    cJSON_AddStringToObject( days, FIELD_TYPE, type_as_string );

    if ( program->days.type == on || program->days.on_days != 0 ) {
        cJSON *days_array = cJSON_AddArrayToObject( days, FIELD_ON_DAYS );
        for ( int day_index = 1; day_index <= VALUE_DAY_NAMES_COUNT; day_index++ ) {
            if ( program->days.on_days & ( 1 << day_index )) {
                cJSON_AddItemToArray( days_array, cJSON_CreateString( VALUE_DAY_NAMES[ day_index ] ));
            }
        }
    }

    if ( program->days.type == interval || program->days.interval_days != 0 ) {
        cJSON_AddNumberToObject( days, FIELD_INTERVAL_DAYS, program->days.interval_days );
    }
    if ( program->days.type == interval || program->days.interval_start_day != 0 ) {
        cJSON_AddStringToObject( days, FIELD_INTERVAL_STARTS_ON,
                                 VALUE_DAY_NAMES[ program->days.interval_start_day ] );
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
        cJSON_AddNumberToObject( item, FIELD_ZONE_ID, zone->zone_id + 1 );
        cJSON_AddNumberToObject( item, FIELD_DURATION, zone->duration_in_seconds );
    }
}

void program_write_to_string( Program *program, char **json_out ) {
    cJSON *root = cJSON_CreateObject();

    write_head( root, program );
    write_days( root, program );
    write_start_times( root, program );
    write_zones( root, program );

    *json_out = cJSON_Print( root );
    cJSON_Delete( root );
}

void programs_header_write_to_json( char **json_out ) {
    cJSON *root = cJSON_CreateArray();

    int count = program_get_count();
    for ( int i = 0; i < count; i++ ) {
        Program *program = program_get( i );
        if ( program == NULL) {
            continue;
        }
        cJSON *item = cJSON_CreateObject();
        cJSON_AddItemToArray( root, item );
        cJSON_AddNumberToObject( item, FIELD_INDEX, program->index );
        cJSON_AddStringToObject( item, FIELD_NAME, program->name );
    }

    *json_out = cJSON_Print( root );
    cJSON_Delete( root );
}

