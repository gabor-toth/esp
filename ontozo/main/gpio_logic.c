#include <stdint.h>
//#include <string.h>
//#include <driver/gpio.h>
//#include <soc/rtc_io_reg.h>
//#include <esp_log.h>
//#include "lib/gpio_task.h"
//#include "lib/common.h"
#include "config.h"
#include "lib/gpio_define.h"
#include "gpio_logic.h"

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

static void set_initial_pump_states() {
    int level0 = gpio_get_level( GPIO_INPUT_LEVEL_1 );
    int level1 = gpio_get_level( GPIO_INPUT_LEVEL_2 );
    int level2 = gpio_get_level( GPIO_INPUT_LEVEL_3 );
    int level3 = gpio_get_level( GPIO_INPUT_LEVEL_4 );

    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_MAIN, false );
    // reached( level0 ) && reached( level1 )
    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_REFILL,
                        !reached( level3 )
                        || !reached( level2 )
                        || !reached( level1 )
                        || !reached( level0 ));
}

void define_output_pins_callback( gpio_config_t *io_conf ) {
    add_output_class( "pumps", 2, high_is_on );
    add_output_class( "zones", 8, low_is_on );

    add_output_pin( PUMPS, GPIO_OUTPUT_PUMP_MAIN, "öntöző", inherit, &io_conf->pin_bit_mask );
    add_output_pin( PUMPS, GPIO_OUTPUT_PUMP_REFILL, "kút", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_1, "fű nagy", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_2, "fű elöl", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_3, "fű hátul", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_4, "hátsó kiskert", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_5, "első kiskert", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_6, "veteményes", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_7, "ribizli", inherit, &io_conf->pin_bit_mask );
    add_output_pin( ZONES, GPIO_OUTPUT_ZONE_8, NULL, inherit, &io_conf->pin_bit_mask );

    set_initial_pump_states();
}

void define_input_pins_callback( gpio_config_t *io_conf ) {
    add_input_class( "levels", 4, low_is_on );
    add_input_class( "buttons", 2, low_is_on );

    add_input_pin( LEVELS, GPIO_INPUT_LEVEL_1, "level1", low_is_on, &io_conf->pin_bit_mask );
    add_input_pin( LEVELS, GPIO_INPUT_LEVEL_2, "level2", low_is_on, &io_conf->pin_bit_mask );
    add_input_pin( LEVELS, GPIO_INPUT_LEVEL_3, "level3", high_is_on, &io_conf->pin_bit_mask );
    add_input_pin( LEVELS, GPIO_INPUT_LEVEL_4, "level4", high_is_on, &io_conf->pin_bit_mask );
    add_input_pin( BUTTONS, GPIO_INPUT_BUTTON_START, "button_start", low_is_on, &io_conf->pin_bit_mask );
    add_input_pin( BUTTONS, GPIO_INPUT_BUTTON_STOP, "button_stop", low_is_on, &io_conf->pin_bit_mask );
}

void gpio_changed_callback( uint32_t io_num, int state ) {
    printf( "GPIO[%d] = %d\n", io_num, state );

    bool refill_state = gpio_get_pin_state( OUTPUTS, PUMPS, PUMP_REFILL );
    bool main_state = gpio_get_pin_state( OUTPUTS, PUMPS, PUMP_MAIN );

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == PIN_ENABLED ) {
            refill_state = false;
        } else {
            refill_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_3 ) {
        // no change for this sensor
    } else if ( io_num == GPIO_INPUT_LEVEL_2 ) {
        if ( state == PIN_ENABLED ) {
            main_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_1 ) {
        if ( state == PIN_DISABLED ) {
            main_state = false;
        }
    } else if ( io_num == GPIO_INPUT_BUTTON_START ) {
        main_state = true;
    } else if ( io_num == GPIO_INPUT_BUTTON_STOP ) {
        main_state = false;
    } else {
        fprintf(stderr, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_MAIN, main_state );
    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_REFILL, refill_state );
}
