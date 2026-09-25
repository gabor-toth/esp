#include "nvs_main.h"
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

static const char* LOG_TAG = "nvs";

#define NVS_DEBUG           1
#define NVS_DEBUG_VALUE     0

void nvs_init( void ) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if ( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_stats_t nvs_stats;
    err = nvs_get_stats( NVS_DEFAULT_PART_NAME, &nvs_stats );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error getting NVS stats: %s", esp_err_to_name( err ) );
    } else {
        ESP_LOGI( LOG_TAG, "namespace_count: %d, total_entries: %d, used_entries: %d, free_entries: %d",
                  nvs_stats.namespace_count,
                  nvs_stats.total_entries,
                  nvs_stats.used_entries,
                  nvs_stats.free_entries
        );
    }
}

nvs_handle_t nvs_open_storage( const char* namespace ) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open( namespace, NVS_READWRITE, &my_handle );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error opening NVS namespace %s: %s", namespace, esp_err_to_name( err ) );
        return 0;
    }
    return my_handle;
}

void nvs_close_storage( nvs_handle_t handle ) {
    nvs_commit( handle );
    nvs_close( handle );
}

char* nvs_read_string( nvs_handle_t nvs_handle, const char* key ) {
    size_t length;
    char* result = NULL;
    esp_err_t err = nvs_get_str( nvs_handle, key, 0, &length );
    if ( err == ESP_ERR_NVS_NOT_FOUND ) {
        result = NULL;
    } else if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error reading length of key %s: %s", key, esp_err_to_name( err ) );
    } else {
        result = malloc( length );
        err = nvs_get_str( nvs_handle, key, result, &length );
        if ( err != ESP_OK ) {
            free( result );
            result = NULL;
            ESP_LOGE( LOG_TAG, "Error reading value of key %s: %s", key, esp_err_to_name( err ) );
        }
    }
#if NVS_DEBUG_VALUE
    ESP_LOGI( LOG_TAG, "Read key %s: %s", key, result != NULL ? result : "NULL" );
#elif NVS_DEBUG
    ESP_LOGI( LOG_TAG, "Read key %s", key );
#endif
    return result;
}

void nvs_open_and_write_string( const char* namespace, const char* key, const char* value ) {
    nvs_handle_t nvs_handle = nvs_open_storage( namespace );
    if ( nvs_handle == 0 ) {
        return;
    }
    nvs_write_string( nvs_handle, key, value );
    nvs_commit( nvs_handle );
    nvs_close( nvs_handle );
}

void nvs_write_string( nvs_handle_t nvs_handle, const char* key, const char* value ) {
    esp_err_t err;
    if ( value != NULL && *value != 0 ) {
#if NVS_DEBUG_VALUE
        ESP_LOGI( LOG_TAG, "Writing key %s: %s", key, value );
#elif NVS_DEBUG
        ESP_LOGI( LOG_TAG, "Writing key %s", key );
#endif
        err = nvs_set_str( nvs_handle, key, value );
    } else {
#if NVS_DEBUG || NVS_DEBUG_VALUE
        ESP_LOGI( LOG_TAG, "Deleting key %s", key );
#endif
        err = nvs_erase_key( nvs_handle, key );
    }
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error writing key %s: %s", key, esp_err_to_name( err ) );
    }
}

void nvs_write_u16( nvs_handle_t nvs_handle, const char* key, uint16_t value ) {
    esp_err_t err = nvs_set_u16( nvs_handle, key, value );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error writing key %s: %s", key, esp_err_to_name( err ) );
    }
}

void nvs_delete( nvs_handle_t nvs_handle, const char* key ) {
#if NVS_DEBUG || NVS_DEBUG_VALUE
    ESP_LOGI( LOG_TAG, "Deleting key %s", key );
#endif

    esp_err_t err = nvs_erase_key( nvs_handle, key );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error erasing key %s: %s", key, esp_err_to_name( err ) );
    }
}
