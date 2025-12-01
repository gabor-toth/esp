#include "config_version.h"
#include "esp_log.h"
#include "sntp_main.h"
#include "nvs_main.h"
#include "time.h"

static const char *TAG = "config_version";

static void set_string( ConfigVersion *version ) {
    snprintf( version->string, sizeof( version->string ), "%llx", version->version );
}

void config_version_set_and_write( uint32_t nvs_handle, ConfigVersion *version ) {
    if ( sntp_is_time_set() ) {
        time( &version->version );
    } else {
        version->version++;
    }
    esp_err_t result = nvs_set_i64( nvs_handle, version->nvs_key, version->version );
    if ( result != ESP_OK ) {
        ESP_LOGE( TAG, "Unable to write version to key %s: %04x", version->nvs_key, result );
    } else {
        ESP_LOGI( TAG, "Wrote version %llx for key %s", version->version, version->nvs_key );
    }
    set_string( version );
}

void config_version_read( uint32_t nvs_handle, const char *nvs_key, ConfigVersion *version ) {
    version->nvs_key = nvs_key;
    esp_err_t result = nvs_get_i64( nvs_handle, version->nvs_key, &version->version );
    if ( result != ESP_OK ) {
        // TODO really log ESP_ERR_NVS_NOT_FOUND?
        ESP_LOGW( TAG, "Unable to read version from key %s: %04x", version->nvs_key, result );
        version->version = 1;
    } else {
        ESP_LOGI( TAG, "Got version %llx for key %s", version->version, version->nvs_key );
    }
    set_string( version );
}
