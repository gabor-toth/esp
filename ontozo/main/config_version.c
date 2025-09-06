#include "config_version.h"
#include "sntp_main.h"
#include "nvs_main.h"
#include "time.h"

static void set_string( ConfigVersion *version ) {
    snprintf( version->string, sizeof( version->string ), "%llx", version->version );
}

void config_version_set_and_write( uint32_t nvs_handle, ConfigVersion *version ) {
    if ( sntp_is_time_set() ) {
        time( &version->version );
    } else {
        version->version++;
    }
    nvs_set_u64( nvs_handle, version->nvs_key, version->version );
    set_string( version );
}

void config_version_read( uint32_t nvs_handle, const char *nvs_key, ConfigVersion *version ) {
    version->nvs_key = nvs_key;
    if ( nvs_get_i64( nvs_handle, version->nvs_key, &version->version ) != ESP_OK ) {
        version->version = 1;
    }
    set_string( version );
}
