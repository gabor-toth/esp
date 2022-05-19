#include "test_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "lib/common.h"
#include "config.h"

static gpio_num_t output_pins[] = {
        GPIO_OUTPUT_ZONE_1,
        GPIO_OUTPUT_ZONE_2,
        GPIO_OUTPUT_ZONE_3,
        GPIO_OUTPUT_ZONE_4,
        GPIO_OUTPUT_ZONE_5,
        GPIO_OUTPUT_ZONE_6,
        GPIO_OUTPUT_ZONE_7,
        GPIO_OUTPUT_ZONE_8,
        GPIO_OUTPUT_PUMP_REFILL,
        GPIO_OUTPUT_PUMP_MAIN,
};
#define NUMBER_OF_PINS  (sizeof( output_pins ) / sizeof( gpio_num_t ))

#define ENABLED_STATE( counter ) ((counter) < 8 ? ZONE_ENABLED : ENABLED)

static unsigned int blink_counter = NUMBER_OF_PINS - 1;

void timer_blink_callback( TimerHandle_t xTimer ) {
    gpio_set_level( output_pins[ blink_counter ], !ENABLED_STATE( blink_counter ));
    blink_counter = ( blink_counter + 1 ) % NUMBER_OF_PINS;
    gpio_set_level( output_pins[ blink_counter ], ENABLED_STATE( blink_counter ));
}

void init_test() {
    TimerHandle_t timer_blink = xTimerCreate(
            "blink",
            1000 / portTICK_PERIOD_MS,
            1,
            NULL,
            timer_blink_callback );
    xTimerReset( timer_blink, 10 );
}

