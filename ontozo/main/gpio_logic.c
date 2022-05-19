#include <stdio.h>
#include <driver/gpio.h>
#include <soc/rtc_io_reg.h>
#include "lib/gpio_task.h"
#include "lib/common.h"
#include "config.h"
#include "gpio_logic.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define reached( X ) ((X)!=0)

static void gpio_changed( uint32_t io_num, int state );

static void set_initial_pump_states();

static int pump_main_state = 0;
static int pump_refill_state = 0;

static void print_state() {
    printf( "levels: 4-%d%d%d%d-1 pumps: %c%c\n",
            gpio_get_level( GPIO_INPUT_LEVEL_4 ),
            gpio_get_level( GPIO_INPUT_LEVEL_3 ),
            gpio_get_level( GPIO_INPUT_LEVEL_2 ),
            gpio_get_level( GPIO_INPUT_LEVEL_1 ),
            pump_main_state ? 'M' : 'm',
            pump_refill_state ? 'R' : 'r' );
}

void gpio_init() {

    /* does not work to get pin 26 work as output
    // Disable DAC1
    REG_CLR_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE );
    // Disable DAC2
    REG_CLR_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC_XPD_FORCE );
     */

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    gpio_set_level( GPIO_OUTPUT_PUMP_MAIN, DISABLED );
    gpio_set_level( GPIO_OUTPUT_PUMP_REFILL, DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_1, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_2, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_3, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_4, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_5, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_6, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_7, ZONE_DISABLED );
    gpio_set_level( GPIO_OUTPUT_ZONE_8, ZONE_DISABLED );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
            ( 1ULL << GPIO_OUTPUT_PUMP_MAIN ) |
            ( 1ULL << GPIO_OUTPUT_PUMP_REFILL ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_1 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_2 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_3 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_4 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_5 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_6 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_7 ) |
            ( 1ULL << GPIO_OUTPUT_ZONE_8 ) |
            0;
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = DISABLED;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask =
            ( 1ULL << GPIO_INPUT_LEVEL_1 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_2 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_3 ) |
            ( 1ULL << GPIO_INPUT_LEVEL_4 ) |
            ( 1ULL << GPIO_INPUT_BUTTON_START ) |
            ( 1ULL << GPIO_INPUT_BUTTON_STOP ) |
            0;
    io_conf.pull_down_en = DISABLED;
    io_conf.pull_up_en = ENABLED;
    gpio_config( &io_conf );

    //install gpio isr service
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );

    gpio_task_init( gpio_changed );
    gpio_task_add( GPIO_INPUT_LEVEL_1 );
    gpio_task_add( GPIO_INPUT_LEVEL_2 );
    gpio_task_add( GPIO_INPUT_LEVEL_3 );
    gpio_task_add( GPIO_INPUT_LEVEL_4 );
    gpio_task_add( GPIO_INPUT_BUTTON_START );
    gpio_task_add( GPIO_INPUT_BUTTON_STOP );

    set_initial_pump_states();
    print_state();
}

static void set_initial_pump_states() {
    int level0 = gpio_get_level( GPIO_INPUT_LEVEL_1 );
    int level1 = gpio_get_level( GPIO_INPUT_LEVEL_2 );
    int level2 = gpio_get_level( GPIO_INPUT_LEVEL_3 );
    int level3 = gpio_get_level( GPIO_INPUT_LEVEL_4 );

    pump_main_state =
            reached( level0 )
            && reached( level1 );
    pump_refill_state =
            !reached( level3 )
            || !reached( level2 )
            || !reached( level1 )
            || !reached( level0 );
    gpio_set_level( GPIO_OUTPUT_PUMP_MAIN, pump_main_state );
    gpio_set_level( GPIO_OUTPUT_PUMP_REFILL, pump_refill_state );
    print_state();
}

static void gpio_changed( uint32_t io_num, int state ) {
    printf( "GPIO[%d] = %d\n", io_num, state );

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == ENABLED ) {
            pump_refill_state = DISABLED;
        } else {
            pump_refill_state = ENABLED;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_3 ) {
        // no change for this sensor
    } else if ( io_num == GPIO_INPUT_LEVEL_2 ) {
        if ( state == ENABLED ) {
            pump_main_state = ENABLED;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_1 ) {
        if ( state == DISABLED ) {
            pump_main_state = DISABLED;
        }
    } else if ( io_num == GPIO_INPUT_BUTTON_START ) {
        pump_main_state = ENABLED;
    } else if ( io_num == GPIO_INPUT_BUTTON_STOP ) {
        pump_main_state = DISABLED;
    } else {
        fprintf(stderr, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    gpio_set_level( GPIO_OUTPUT_PUMP_MAIN, pump_main_state );
    gpio_set_level( GPIO_OUTPUT_PUMP_REFILL, pump_refill_state );
    print_state();
}
