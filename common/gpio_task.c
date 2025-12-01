#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "gpio_task.h"

static const char *TAG = "gpio_task";

#define DEFAULT_DELAY_MS 100

typedef struct {
    gpio_num_t io_num;
    TimerHandle_t timer_going_low;
    TimerHandle_t timer_going_high;
    int last_reported_state;
} GpioTimer;

static QueueHandle_t gpio_evt_queue = NULL;
static gpio_change_callback change_callback = NULL;

static void IRAM_ATTR gpio_isr_handler( void *arg ) {
    GpioTimer *timer_data = (GpioTimer *) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if ( timer_data->last_reported_state ) {
        if ( xTimerResetFromISR( timer_data->timer_going_low, &xHigherPriorityTaskWoken ) != pdPASS ) {
            ESP_DRAM_LOGE( TAG, "gpio_isr_handler" );
        }
    } else {
        if ( xTimerResetFromISR( timer_data->timer_going_high, &xHigherPriorityTaskWoken ) != pdPASS ) {
            ESP_DRAM_LOGE( TAG, "gpio_isr_handler" );
        }
    }
}

static void timer_gpio_callback( TimerHandle_t timer ) {
    GpioTimer *timer_data = (GpioTimer *) pvTimerGetTimerID( timer );
    ESP_LOGI( TAG, "timer %d", timer_data->io_num );

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStopFromISR( timer_data->timer_going_high, &xHigherPriorityTaskWoken );
    xTimerStopFromISR( timer_data->timer_going_low, &xHigherPriorityTaskWoken );

    int current_state = gpio_get_level( timer_data->io_num );
    if ( current_state != timer_data->last_reported_state ) {
        timer_data->last_reported_state = current_state;
        xQueueSendToBack( gpio_evt_queue, &timer_data, portMAX_DELAY );
    }
}

_Noreturn static void task_gpio( void *arg ) {
    for ( ;; ) {
        GpioTimer *timer_data;
        if ( xQueueReceive( gpio_evt_queue, &timer_data, portMAX_DELAY ) ) {
            change_callback( timer_data->io_num, timer_data->last_reported_state );
        }
    }
}

void gpio_task_init( gpio_change_callback _change_callback ) {
    change_callback = _change_callback;
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate( 10, sizeof( GpioTimer * ) );
    //start gpio task
    xTaskCreate( task_gpio, "task_gpio", 3072, NULL, 10, NULL );
}

static TimerHandle_t create_timer( const char *timer_name, const GpioTimer *timer_data, int delay_ms ) {
    return xTimerCreate(
            timer_name,
            pdMS_TO_TICKS( delay_ms != 0 ? delay_ms : DEFAULT_DELAY_MS ),
            0,
            (void *) timer_data,
            timer_gpio_callback );
}

void gpio_task_add( gpio_num_t io_num, int delay_ms_on_going_low, int delay_ms_on_going_high ) {
    char timer_name[10];

    sprintf( timer_name, "gpio%d", io_num );
    GpioTimer *timer_data = malloc( sizeof( GpioTimer ) );
    timer_data->io_num = io_num;
    timer_data->last_reported_state = gpio_get_level( io_num );
    timer_data->timer_going_high = create_timer( timer_name, timer_data, delay_ms_on_going_high );
    if ( delay_ms_on_going_high == delay_ms_on_going_low ) {
        timer_data->timer_going_low = timer_data->timer_going_high;
    } else {
        timer_data->timer_going_low = create_timer( timer_name, timer_data, delay_ms_on_going_low );
    }
    ESP_ERROR_CHECK( gpio_isr_handler_add( io_num, gpio_isr_handler, (void *) timer_data ) );
}
