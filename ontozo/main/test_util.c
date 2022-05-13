#include "test_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

static int blink_counter = 0;

void timer_blink_callback( TimerHandle_t xTimer ) {
    printf( "blink_counter: %d\n", blink_counter );
    gpio_set_level( CONFIG_GPIO_OUTPUT_0, blink_counter % 2 );
    gpio_set_level( CONFIG_GPIO_OUTPUT_1, 1 - ( blink_counter % 2 ));
    blink_counter++;
}

void init_test() {
    TimerHandle_t timer_blink = xTimerCreate( "blink", 1000 / portTICK_PERIOD_MS, 1, NULL, timer_blink_callback );
    xTimerReset( timer_blink, 10 );
}

