#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#include "gpio_task.h"

static QueueHandle_t gpio_evt_queue = NULL;
static gpio_change_callback change_callback;

static GpioTimer gpio_timers[] =
        {
                { CONFIG_GPIO_INPUT_0, NULL, 0 },
                { CONFIG_GPIO_INPUT_1, NULL, 0 },
        };

static void IRAM_ATTR gpio_isr_handler( void *arg ) {
    uint32_t gpio_index = (uint32_t) arg;
    xTimerResetFromISR( gpio_timers[ gpio_index ].timer, NULL );
}

void timer_gpio_callback( TimerHandle_t timer ) {
    uint32_t gpio_index = (uint32_t) pvTimerGetTimerID( timer );
    uint32_t io_num = gpio_timers[ gpio_index ].io_num;
    int current_state = gpio_get_level( io_num );
    if ( current_state != gpio_timers[ gpio_index ].last_reported_state ) {
        gpio_timers[ gpio_index ].last_reported_state = current_state;
        xQueueSend( gpio_evt_queue, &io_num, 0 );
    }
}

_Noreturn static void task_gpio( void *arg ) {
    char timer_name[10];

    for ( int i = 0; i < sizeof( gpio_timers ) / sizeof( GpioTimer ); i++ ) {
        sprintf( timer_name, "gpio%d", i );
        gpio_timers[ i ].timer = xTimerCreate(
                timer_name,
                100 / portTICK_PERIOD_MS,
                0,
                (void *) i,
                timer_gpio_callback );
        uint32_t io_num = gpio_timers[ i ].io_num;
        gpio_timers[ i ].last_reported_state = gpio_get_level( io_num );
        gpio_isr_handler_add( io_num, gpio_isr_handler, (void *) i );
    }

    for ( ;; ) {
        uint32_t io_num;
        if ( xQueueReceive( gpio_evt_queue, &io_num, portMAX_DELAY )) {
            change_callback( io_num, gpio_get_level( io_num ));
        }
    }
}

void gpio_task_init( gpio_change_callback _change_callback ) {
    change_callback = _change_callback;
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate( 10, sizeof( uint32_t ));
    //start gpio task
    xTaskCreate( task_gpio, "task_gpio", 2048, NULL, 10, NULL);
}
