#include <cJSON.h>
#include <string.h>
#include "gpio_json.h"

#define FIELD_NAME  "name"
#define FIELD_MANUAL  "manual"

char *gpio_data_to_json_string( PinData *config ) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject( root, FIELD_NAME, config->name );
    cJSON_AddBoolToObject( root, FIELD_MANUAL, config->is_manual );

    char *json_string = cJSON_Print( root );
    cJSON_Delete( root );
    return json_string;
}

void gpio_data_from_json_string( const char *json_string, PinData *config ) {
    cJSON *root = cJSON_Parse( json_string );
    cJSON *item;

    item = cJSON_GetObjectItem( root, FIELD_NAME );
    if ( cJSON_IsString( item )) {
        config->name = strdup( item->valuestring );
    }
    item = cJSON_GetObjectItem( root, FIELD_MANUAL );
    if ( cJSON_IsBool( item )) {
        config->is_manual = item->valueint;
    }

    cJSON_Delete( root );
}
