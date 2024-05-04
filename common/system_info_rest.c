#include "sdkconfig.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)

#include "http/http_server.h"
#include "rest_util.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include <esp_chip_info.h>
#include "system_info_rest.h"

static const char *TAG = "rest_sysinfo";

__attribute__((unused))
static void add_task_list( cJSON *root ) {
    UBaseType_t numberOfTasks = uxTaskGetNumberOfTasks();
    unsigned long ulTotalRunTime, ulStatsAsPercentage;
    TaskStatus_t *pxTaskStatusArray = malloc( numberOfTasks * sizeof( TaskStatus_t ));

    uxTaskGetSystemState( pxTaskStatusArray, numberOfTasks, &ulTotalRunTime );
    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;

    cJSON *jsonTasks = cJSON_AddObjectToObject( root, "tasks" );
    cJSON_AddNumberToObject( jsonTasks, "runtime", ulTotalRunTime );
    cJSON *jsonTaskList = cJSON_AddArrayToObject( jsonTasks, "taskList" );
    /* Avoid divide by zero errors. */
    if ( ulTotalRunTime > 0 ) {
        /* For each populated position in the pxTaskStatusArray array,
        format the raw data as human-readable ASCII data. */
        for ( UBaseType_t x = 0; x < numberOfTasks; x++ ) {
            /* What percentage of the total run time has the task used?
            This will always be rounded down to the nearest integer.
            ulTotalRunTimeDiv100 has already been divided by 100. */
            ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;
            cJSON *jsonTask = cJSON_CreateObject();
            cJSON_AddItemToArray( jsonTaskList, jsonTask );
            cJSON_AddStringToObject( jsonTask, "name", pxTaskStatusArray[ x ].pcTaskName );
            cJSON_AddNumberToObject( jsonTask, "runtime", pxTaskStatusArray[ x ].ulRunTimeCounter );
            cJSON_AddNumberToObject( jsonTask, "percentage", ulStatsAsPercentage );
        }
    }

    /* The array is no longer needed, free the memory it consumes. */
    free( pxTaskStatusArray );
}

static void add_heap_info( cJSON *root ) {
    cJSON *jsonHeap = cJSON_AddObjectToObject( root, "heap" );
    multi_heap_info_t heap_info;
    heap_caps_get_info( &heap_info, MALLOC_CAP_8BIT);
    cJSON_AddNumberToObject( jsonHeap, "allocatedBytes", heap_info.total_allocated_bytes );
    cJSON_AddNumberToObject( jsonHeap, "freeBytes", heap_info.total_free_bytes );
//    heap_caps_get_total_size
//    heap_caps_get_free_size
}

static void add_chip_info( cJSON *root ) {
    esp_chip_info_t chip_info;
    esp_chip_info( &chip_info );
    cJSON *jsonChip = cJSON_AddObjectToObject( root, "chip" );
    cJSON_AddNumberToObject( jsonChip, "cores", chip_info.cores );
    /*
    typedef enum {
        CHIP_ESP32  = 1, //!< ESP32
        CHIP_ESP32S2 = 2, //!< ESP32-S2
        CHIP_ESP32S3 = 9, //!< ESP32-S3
        CHIP_ESP32C3 = 5, //!< ESP32-C3
        CHIP_ESP32H2 = 6, //!< ESP32-H2
        CHIP_ESP32C2 = 12, //!< ESP32-C2
    } esp_chip_model_t;
     */
    cJSON_AddNumberToObject( jsonChip, "revision", chip_info.revision );
    cJSON_AddNumberToObject( jsonChip, "model", chip_info.model );
    /*
    #define CHIP_FEATURE_EMB_FLASH      BIT(0)      //!< Chip has embedded flash memory
    #define CHIP_FEATURE_WIFI_BGN       BIT(1)      //!< Chip has 2.4GHz WiFi
    #define CHIP_FEATURE_BLE            BIT(4)      //!< Chip has Bluetooth LE
    #define CHIP_FEATURE_BT             BIT(5)      //!< Chip has Bluetooth Classic
    #define CHIP_FEATURE_IEEE802154     BIT(6)      //!< Chip has IEEE 802.15.4
    #define CHIP_FEATURE_EMB_PSRAM      BIT(7)      //!< Chip has embedded psram
     */
    cJSON_AddNumberToObject( jsonChip, "features", chip_info.features );
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler( httpd_req_t *req ) {
    rest_allow_cors( req );
    httpd_resp_set_type( req, "application/json" );
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "idfVersion", IDF_VER );
    add_chip_info( root );
    add_heap_info( root );
// TODO   add_task_list( root );
    const char *sys_info = cJSON_Print( root );
    httpd_resp_sendstr( req, sys_info );
    free((void *) sys_info );
    cJSON_Delete( root );
    return ESP_OK;
}

void rest_register_system_info_handler(httpd_handle_t server, http_server_context_t *rest_context ) {
    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
            .uri = "/system/info",
            .method = HTTP_GET,
            .handler = system_info_get_handler,
            .user_ctx = rest_context
    };
    http_register_uri_handler(server, TAG, &system_info_get_uri);
}

#endif
