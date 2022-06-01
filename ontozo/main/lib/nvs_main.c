#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "nvs_main.h"

#define STORAGE_NAMESPACE "storage"
#define LOG_TAG "nvs"

void nvs_init( void ) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if ( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK( nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_stats_t nvs_stats;
    err = nvs_get_stats( NVS_DEFAULT_PART_NAME, &nvs_stats );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error getting NVS stats on %s: %s", STORAGE_NAMESPACE, esp_err_to_name( err ));
    } else {
        ESP_LOGI( LOG_TAG, "namespace_count: %d, total_entries: %d, used_entries: %d, free_entries: %d",
                  nvs_stats.namespace_count,
                  nvs_stats.total_entries,
                  nvs_stats.used_entries,
                  nvs_stats.free_entries
        );
    }
}

nvs_handle_t nvs_open_storage() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error opening NVS %s: %s", STORAGE_NAMESPACE, esp_err_to_name( err ));
        return 0;
    }
    return my_handle;
}

char *nvs_read_string( const char *key ) {
    nvs_handle_t my_handle = nvs_open_storage();
    if ( my_handle == 0 ) {
        return NULL;
    }

    size_t length;
    char *result = NULL;
    esp_err_t err = nvs_get_str( my_handle, key, 0, &length );
    if ( err == ESP_ERR_NVS_NOT_FOUND) {
        result = NULL;
    } else if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error reading length of key %s: %s", key, esp_err_to_name( err ));
    } else {
        result = malloc( length );
        err = nvs_get_str( my_handle, key, result, &length );
        if ( err != ESP_OK ) {
            free( result );
            result = NULL;
            ESP_LOGE( LOG_TAG, "Error reading value of key %s: %s", key, esp_err_to_name( err ));
        }
    }
    ESP_LOGI( LOG_TAG, "Read key %s: %s", key, result != NULL ? result : "NULL" );
    nvs_close( my_handle );
    return result;
}

void nvs_write_string( const char *key, const char *value ) {
    ESP_LOGI( LOG_TAG, "Writing key %s: %s", key, value );

    nvs_handle_t my_handle = nvs_open_storage();
    if ( my_handle == 0 ) {
        return;
    }

    esp_err_t err = nvs_set_str( my_handle, key, value );
    if ( err != ESP_OK ) {
        ESP_LOGE( LOG_TAG, "Error writing of key %s: %s", key, esp_err_to_name( err ));
    } else {
        nvs_commit( my_handle );
    }
    nvs_close( my_handle );
}
