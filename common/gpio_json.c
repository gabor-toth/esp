#include <cJSON.h>
#include <string.h>
#include "gpio_json.h"

char* gpio_data_to_json_string(PinData* config)
{
    cJSON* root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, GPIO_FIELD_HIDDEN, config->is_hidden);
    cJSON_AddBoolToObject(root, GPIO_FIELD_INACTIVE, config->is_inactive);
    cJSON_AddBoolToObject(root, GPIO_FIELD_MANUAL, config->is_manual);
    cJSON_AddStringToObject(root, GPIO_FIELD_NAME, config->name);

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}

void gpio_data_from_json_string(const char* json_string, PinData* config)
{
    cJSON* root = cJSON_Parse(json_string);
    cJSON* item;

    item = cJSON_GetObjectItem(root, GPIO_FIELD_HIDDEN);
    config->is_hidden = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(root, GPIO_FIELD_INACTIVE);
    config->is_inactive = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(root, GPIO_FIELD_MANUAL);
    config->is_manual = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(root, GPIO_FIELD_NAME);
    if (cJSON_IsString(item))
    {
        config->name = strdup(item->valuestring);
    }

    cJSON_Delete(root);
}
