#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "gpio_task.h"
#include "test_util.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define ENABLED     1
#define DISABLED    0

void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] intr, val: %d\n", io_num, state );
}

static void init_gpio() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ( 1ULL << CONFIG_GPIO_OUTPUT_0 ) | ( 1ULL << CONFIG_GPIO_OUTPUT_1 );
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = DISABLED;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask =
            ( 1ULL << GPIO_INPUT_LEVEL_0 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_1 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_2 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_3 ) |
            ( 1ULL << GPIO_INPUT_BUTTON_START ) |
            ( 1ULL << GPIO_INPUT_BUTTON_STOP );
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = ENABLED;
    gpio_config( &io_conf );

    //install gpio isr service
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );

    gpio_task_init( gpio_changed );
    gpio_task_add( GPIO_INPUT_LEVEL_0 );
    gpio_task_add( GPIO_INPUT_LEVEL_1 );
    gpio_task_add( GPIO_INPUT_LEVEL_2 );
    gpio_task_add( GPIO_INPUT_LEVEL_3 );
    gpio_task_add( GPIO_INPUT_BUTTON_START );
    gpio_task_add( GPIO_INPUT_BUTTON_STOP );
}

void app_main( void ) {
    init_gpio();
    init_test();
}
