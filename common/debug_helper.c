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

void debug_start_task_dump( uint16_t period_sec ) {
    esp_timer_create_args_t args = {
            .callback = task_dump,
    };
    esp_timer_handle_t timer = NULL;
    esp_timer_create( &args, &timer );
    esp_timer_start_periodic( timer, period_sec * (uint64_t) 1000000 );
}

static void heap_dump( void *arg ) {
    (void) arg;

    debug_print_free_mem( LOG );
}

void debug_start_heap_dump( uint16_t period_sec ) {
    esp_timer_create_args_t args = {
            .callback = heap_dump,
    };
    esp_timer_handle_t timer = NULL;
    esp_timer_create( &args, &timer );
    esp_timer_start_periodic( timer, period_sec * (uint64_t) 1000000 );
}

size_t debug_print_free_mem( const char *log ) {
    size_t free_size = heap_caps_get_free_size( MALLOC_CAP_8BIT );
    size_t largest_free_size = heap_caps_get_largest_free_block( MALLOC_CAP_8BIT );
    ESP_LOGI( log != NULL ? log : LOG, "free mem %d largest %d",
              free_size, largest_free_size );
    return free_size;
}
