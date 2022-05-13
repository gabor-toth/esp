#include <stdio.h>
#include <driver/gpio.h>
#include "lib/gpio_task.h"
#include "config.h"
#include "gpio_logic.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define ENABLED     1
#define DISABLED    0

#define reached( X ) ((X)!=0)

static void gpio_changed( uint32_t io_num, int state );

static void set_initial_pump_states();

static int pump_main_state = 0;
static int pump_refill_state = 0;

void init_gpio() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    gpio_set_level( GPIO_OUTPUT_PUMP_MAIN, DISABLED );
    gpio_set_level( GPIO_OUTPUT_PUMP_REFILL, DISABLED );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ( 1ULL << GPIO_OUTPUT_PUMP_MAIN ) | ( 1ULL << GPIO_OUTPUT_PUMP_REFILL );
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

    set_initial_pump_states();
}

static void print_state() {
    printf( "levels: %d%d%d%d pumps: %c%c\n",
            gpio_get_level( GPIO_INPUT_LEVEL_3 ),
            gpio_get_level( GPIO_INPUT_LEVEL_2 ),
            gpio_get_level( GPIO_INPUT_LEVEL_1 ),
            gpio_get_level( GPIO_INPUT_LEVEL_0 ),
            pump_main_state ? 'M' : 'm',
            pump_refill_state ? 'R' : 'r' );
}

static void set_initial_pump_states() {
    int level0 = gpio_get_level( GPIO_INPUT_LEVEL_0 );
    int level1 = gpio_get_level( GPIO_INPUT_LEVEL_1 );
    int level2 = gpio_get_level( GPIO_INPUT_LEVEL_2 );
    int level3 = gpio_get_level( GPIO_INPUT_LEVEL_3 );

    pump_main_state = reached( level0 );
    pump_refill_state = !reached( level3 ) || !reached( level2 ) || !reached( level1 ) || !reached( level0 );
    gpio_set_level( GPIO_OUTPUT_PUMP_MAIN, pump_main_state );
    gpio_set_level( GPIO_OUTPUT_PUMP_REFILL, pump_refill_state );
    print_state();
}

static void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] intr, val: %d\n", io_num, state );
    set_initial_pump_states();
}
