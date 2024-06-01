#include "debug_helper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *LOG = "debug";

static void task_dump( void *arg ) {
    (void) arg;

    UBaseType_t numberOfTasks = uxTaskGetNumberOfTasks();
    unsigned long ulTotalRunTime;
    TaskStatus_t *pxTaskStatusArray = malloc( numberOfTasks * sizeof( TaskStatus_t ) );

    uxTaskGetSystemState( pxTaskStatusArray, numberOfTasks, &ulTotalRunTime );
    ulTotalRunTime /= 100UL;
    for ( int i = 0; i < numberOfTasks; i++ ) {
        TaskStatus_t *task = pxTaskStatusArray + i;
        int percentage = (int) ( task->ulRunTimeCounter / ulTotalRunTime );
        if ( percentage == 0 ) {
            continue;
        }
        ESP_LOGI( LOG, "%8ld %3d %s",
                  task->ulRunTimeCounter,
                  ulTotalRunTime != 0 ? percentage : 0,
                  task->pcTaskName );
    }
    free( pxTaskStatusArray );
}

void debug_start_task_dump() {
    esp_timer_create_args_t args = {
            .callback = task_dump,
    };
    esp_timer_handle_t timer = NULL;
    esp_timer_create( &args, &timer );
    esp_timer_start_periodic( timer, 1000000 );
}

size_t debug_print_free_mem( const char *log ) {
    size_t free_size = heap_caps_get_free_size( MALLOC_CAP_8BIT );
    ESP_LOGW( log != NULL ? log : LOG, "free mem %d",
              free_size );
    return free_size;
}
