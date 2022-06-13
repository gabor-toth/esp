#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#include "gpio_task.h"

#define DEFAULT_DELAY_MS 100

typedef struct {
    uint32_t io_num;
    TimerHandle_t timer_going_low;
    TimerHandle_t timer_going_high;
    int last_reported_state;
} GpioTimer;

static QueueHandle_t gpio_evt_queue = NULL;
static gpio_change_callback change_callback = NULL;

static void IRAM_ATTR gpio_isr_handler( void *arg ) {
    GpioTimer *timer_data = (GpioTimer *) arg;
    xTimerResetFromISR( timer_data->timer_going_high, NULL );
    xTimerResetFromISR( timer_data->timer_going_low, NULL );
}

void timer_gpio_callback( TimerHandle_t timer ) {
    GpioTimer *timer_data = (GpioTimer *) pvTimerGetTimerID( timer );
    uint32_t io_num = timer_data->io_num;
    int current_state = gpio_get_level( io_num );
    if ( current_state != timer_data->last_reported_state ) {
        timer_data->last_reported_state = current_state;
        xQueueSend( gpio_evt_queue, &io_num, 0 );
    }
}

_Noreturn static void task_gpio( void *arg ) {
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

static TimerHandle_t create_timer( const char *timer_name, const GpioTimer *timer_data, int delay_ms ) {
    return xTimerCreate(
            timer_name,
            ( delay_ms != 0 ? delay_ms : DEFAULT_DELAY_MS ) / portTICK_PERIOD_MS,
            0,
            (void *) timer_data,
            timer_gpio_callback );
}

void gpio_task_add( int io_num, int delay_ms_on_going_low, int delay_ms_on_going_high ) {
    char timer_name[10];

    sprintf( timer_name, "gpio%d", io_num );
    GpioTimer *timer_data = malloc( sizeof( GpioTimer ));
    timer_data->io_num = io_num;
    timer_data->last_reported_state = gpio_get_level( io_num );
    timer_data->timer_going_high = create_timer( timer_name, timer_data, delay_ms_on_going_high );
    if ( delay_ms_on_going_high == delay_ms_on_going_low ) {
        timer_data->timer_going_low = timer_data->timer_going_high;
    } else {
        timer_data->timer_going_low = create_timer( timer_name, timer_data, delay_ms_on_going_low );
    }
    gpio_isr_handler_add( io_num, gpio_isr_handler, (void *) timer_data );
}
