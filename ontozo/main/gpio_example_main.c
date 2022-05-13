#include <sys/cdefs.h>
/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#include "gpio_task.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output (ESP32C2/ESP32H2 uses GPIO8 as the second output pin)
 * GPIO19: output (ESP32C2/ESP32H2 uses GPIO9 as the second output pin)
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Note. These are the default GPIO pins to be used in the example. You can
 * change IO pins in menuconfig.
 *
 * Test:
 * Connect GPIO18(8) with GPIO4
 * Connect GPIO19(9) with GPIO5
 * Generate pulses on GPIO18(8)/19(9), that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0    CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1    CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

#define ENABLED     1
#define DISABLED    0

static int blink_counter = 0;

void timer_blink_callback( TimerHandle_t xTimer ) {
    printf( "blink_counter: %d\n", blink_counter );
    gpio_set_level( GPIO_OUTPUT_IO_0, blink_counter % 2 );
    gpio_set_level( GPIO_OUTPUT_IO_1, 1 - ( blink_counter % 2 ));
    blink_counter++;
}

IRAM_ATTR

void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] intr, val: %d\n", io_num, state );
}

void app_main( void ) {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = DISABLED;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = ENABLED;
    gpio_config( &io_conf );

    gpio_task_init( gpio_changed );

    //install gpio isr service
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );

    printf( "Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    TimerHandle_t timer_blink = xTimerCreate( "blink", 1000 / portTICK_PERIOD_MS, 1, NULL, timer_blink_callback );
    xTimerReset( timer_blink, 10 );
}
