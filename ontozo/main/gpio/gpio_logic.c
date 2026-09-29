#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include "../config.h"
#include "gpio_define.h"
#include "gpio_logic.h"
#include "../program/program_logic.h"

static const char* LOG_TAG = "pin_logic";

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

static char* const PUMPS_NAME = "pumps";
static char* const ZONES_NAME = "zones";

static void set_initial_pump_states() {
    int level1 = gpio_get_pin_state( INPUTS, PIN_CLASS_IN_LEVELS, 0 );
    int level2 = gpio_get_pin_state( INPUTS, PIN_CLASS_IN_LEVELS, 1 );
    int level3 = gpio_get_pin_state( INPUTS, PIN_CLASS_IN_LEVELS, 2 );
    int level4 = gpio_get_pin_state( INPUTS, PIN_CLASS_IN_LEVELS, 3 );

    gpio_set_pin_state_forced( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_MAIN, false );
    gpio_set_pin_state_forced( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_REFILL, false );

    bool refillState = !reached( level4 )
        //                       || !reached( level3 )
        //                       || !reached( level2 )
        //                       || !reached( level1 )
        ;
    ESP_LOGI( LOG_TAG, "Levels: 1-%d 2-%d 3-%d 4-%d refillPump-%d", level1, level2, level3, level4, refillState );
    gpio_set_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_REFILL,
                        refillState );
}

static gpio_num_t pump_pins[ ] = {
    GPIO_OUTPUT_PUMP_MAIN,
    GPIO_OUTPUT_PUMP_REFILL,
};
static gpio_num_t zone_pins[ ] = {
    GPIO_OUTPUT_ZONE_1,
    GPIO_OUTPUT_ZONE_2,
    GPIO_OUTPUT_ZONE_3,
    GPIO_OUTPUT_ZONE_4,
    GPIO_OUTPUT_ZONE_5,
    GPIO_OUTPUT_ZONE_6,
    GPIO_OUTPUT_ZONE_7,
    GPIO_OUTPUT_ZONE_8,
    // GPIO_OUTPUT_ZONE_9,
    // GPIO_OUTPUT_ZONE_10,
    // GPIO_OUTPUT_ZONE_11,
    // GPIO_OUTPUT_ZONE_12,
    // GPIO_OUTPUT_ZONE_13,
    // GPIO_OUTPUT_ZONE_14,
    // GPIO_OUTPUT_ZONE_15,
    // GPIO_OUTPUT_ZONE_16,
};

int classLevels;
int classButtons;

void gpio_define_output_pins_callback( gpio_config_t* io_conf, void* user_context ) {
    gpio_add_class( OUTPUTS, PUMPS_NAME, 2, high_is_on );
    gpio_add_class( OUTPUTS, ZONES_NAME, 8, low_is_on );
    gpio_add_class( OUTPUTS, "buttons_out", 1, high_is_on );

    for ( int i = 0; i < sizeof pump_pins / sizeof pump_pins[ 0 ]; i++ ) {
        gpio_add_pin( OUTPUTS, PIN_CLASS_OUT_PUMPS, pump_pins[ i ],
                      inherit,
                      &io_conf->pin_bit_mask );
    }
    for ( int i = 0; i < sizeof zone_pins / sizeof zone_pins[ 0 ]; i++ ) {
        gpio_add_pin( OUTPUTS, PIN_CLASS_OUT_ZONES, zone_pins[ i ],
                      inherit,
                      &io_conf->pin_bit_mask );
    }

    gpio_add_pin( OUTPUTS, PIN_CLASS_OUT_BUTTONS, GPIO_INPUT_BUTTON_2,
                  inherit, &io_conf->pin_bit_mask );

    set_initial_pump_states();
}

void gpio_define_input_pins_callback( gpio_config_t* io_conf, void* user_context ) {
    classLevels = gpio_add_class( INPUTS, "levels", 4, low_is_on );
    classButtons = gpio_add_class( INPUTS, "buttons", 2, low_is_on );

    int index;
    gpio_add_pin( INPUTS, PIN_CLASS_IN_LEVELS, GPIO_INPUT_LEVEL_1,
                  low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, PIN_CLASS_IN_LEVELS, GPIO_INPUT_LEVEL_2,
                  low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, PIN_CLASS_IN_LEVELS, GPIO_INPUT_LEVEL_3,
                  high_is_on, &io_conf->pin_bit_mask );
    index = gpio_add_pin( INPUTS, PIN_CLASS_IN_LEVELS, GPIO_INPUT_LEVEL_4,
                          high_is_on, &io_conf->pin_bit_mask );
    gpio_set_delays( INPUTS, PIN_CLASS_IN_LEVELS, index, 10 * 1000, 10 * 1000 );
    // gpio_add_pin( INPUTS, BUTTONS_CLASS, GPIO_INPUT_BUTTON_2,
    //               low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, PIN_CLASS_IN_BUTTONS, GPIO_INPUT_BUTTON_1,
                  low_is_on, &io_conf->pin_bit_mask );
}

static void gpio_changed_callback( gpio_num_t io_num, int state ) {
    ESP_LOGI( LOG_TAG, "Pin %d changed to %d", io_num, state );

    bool refill_state = gpio_get_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_REFILL );
    bool old_refill_state = refill_state;
    bool main_state = gpio_get_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_MAIN );
    bool old_main_state = main_state;

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == PIN_ENABLED ) {
            refill_state = false;
        } else {
            refill_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_3 || io_num == GPIO_INPUT_LEVEL_2 ) {
        if ( state == PIN_ENABLED ) {
            main_state = true;
        }
    } else if ( io_num == GPIO_INPUT_LEVEL_1 ) {
        if ( state == PIN_DISABLED ) {
            main_state = false;
        }
    } else if ( io_num == GPIO_INPUT_BUTTON_1 ) {
        if ( state ) {
            main_state = !main_state;
        }
    } else {
        ESP_LOGE( LOG_TAG, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    if ( old_main_state != main_state ) {
        ESP_LOGI( LOG_TAG, "Turning main pum %s", main_state ? "on" : "off" );
        gpio_set_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_MAIN, main_state );
        program_logic_pump_state_change( main_state );
        gpio_set_pin_state( OUTPUTS, PIN_CLASS_OUT_BUTTONS, 0, main_state );
    }
    if ( old_refill_state != refill_state ) {
        ESP_LOGI( LOG_TAG, "Turning refill pump %s", refill_state ? "on" : "off" );
        gpio_set_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_REFILL, refill_state );
    }
}

void gpio_logic_init() {
    gpio_init( NULL, gpio_changed_callback );
}

void gpio_pump_main( bool on ) {
    gpio_set_pin_state( OUTPUTS, PIN_CLASS_OUT_PUMPS, PUMP_MAIN, on );
}
