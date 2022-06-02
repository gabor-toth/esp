#include <stdint.h>
#include <esp_log.h>
#include "config.h"
#include "lib/gpio_define.h"
#include "lib/nvs_main.h"
#include "gpio_logic.h"

static const char *LOG_TAG = "logic";

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

static void set_initial_pump_states() {
    int level1 = gpio_get_pin_state( INPUTS, LEVELS, 0 );
    int level2 = gpio_get_pin_state( INPUTS, LEVELS, 1 );
    int level3 = gpio_get_pin_state( INPUTS, LEVELS, 2 );
    int level4 = gpio_get_pin_state( INPUTS, LEVELS, 3 );

    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_MAIN, false );
    // reached( level0 ) && reached( level1 )
    bool refillState = !reached( level4 )
                       || !reached( level3 )
                       || !reached( level2 )
                       || !reached( level1 );
    ESP_LOGI( LOG_TAG, "Levels: 1-%d 2-%d 3-%d 4-%d refillPump-%d", level1, level2, level3, level4, refillState );
    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_REFILL,
                        refillState );
}

static gpio_num_t pump_pins[] = {
        GPIO_OUTPUT_PUMP_MAIN,
        GPIO_OUTPUT_PUMP_REFILL,
};
static gpio_num_t zone_pins[] = {
        GPIO_OUTPUT_ZONE_1,
        GPIO_OUTPUT_ZONE_2,
        GPIO_OUTPUT_ZONE_3,
        GPIO_OUTPUT_ZONE_4,
        GPIO_OUTPUT_ZONE_5,
        GPIO_OUTPUT_ZONE_6,
        GPIO_OUTPUT_ZONE_7,
        GPIO_OUTPUT_ZONE_8,
};

void gpio_define_output_pins_callback( gpio_config_t *io_conf ) {
    gpio_add_class( OUTPUTS, "pumps", 2, high_is_on );
    gpio_add_class( OUTPUTS, "zones", 8, low_is_on );

    char nvs_key[256];
    for ( int i = 0; i < sizeof pump_pins / sizeof pump_pins[ 0 ]; i++ ) {
        snprintf( nvs_key, sizeof nvs_key, "%s.%d.name", "pumps", i + 1 );
        gpio_add_pin_with_allocated_name( OUTPUTS, PUMPS, pump_pins[ i ], nvs_read_string( nvs_key ), inherit,
                                          &io_conf->pin_bit_mask );
    }
    for ( int i = 0; i < sizeof zone_pins / sizeof zone_pins[ 0 ]; i++ ) {
        snprintf( nvs_key, sizeof nvs_key, "%s.%d.name", "zones", i + 1 );
        gpio_add_pin_with_allocated_name( OUTPUTS, ZONES, zone_pins[ i ], nvs_read_string( nvs_key ), inherit,
                                          &io_conf->pin_bit_mask );
    }
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_1, "fű nagy", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_2, "fű elöl", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_3, "fű hátul", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_4, "hátsó kiskert", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_5, "első kiskert", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_6, "veteményes", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_7, "ribizli", inherit, &io_conf->pin_bit_mask );
//    gpio_add_pin( OUTPUTS, ZONES, GPIO_OUTPUT_ZONE_8, NULL, inherit, &io_conf->pin_bit_mask );

    set_initial_pump_states();
}

void gpio_define_input_pins_callback( gpio_config_t *io_conf ) {
    gpio_add_class( INPUTS, "levels", 4, low_is_on );
    gpio_add_class( INPUTS, "buttons", 2, low_is_on );

    gpio_add_pin( INPUTS, LEVELS, GPIO_INPUT_LEVEL_1, "level1", low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, LEVELS, GPIO_INPUT_LEVEL_2, "level2", low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, LEVELS, GPIO_INPUT_LEVEL_3, "level3", high_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, LEVELS, GPIO_INPUT_LEVEL_4, "level4", high_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, BUTTONS, GPIO_INPUT_BUTTON_START, "button_start", low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, BUTTONS, GPIO_INPUT_BUTTON_STOP, "button_stop", low_is_on, &io_conf->pin_bit_mask );
}

void gpio_changed_callback( uint32_t io_num, int state ) {
    ESP_LOGI( LOG_TAG, "Pin %d changed to %d\n", io_num, state );

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
        ESP_LOGE( LOG_TAG, "Unhandled gpio %d (state %d)!\n", io_num, state );
    }

    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_MAIN, main_state );
    gpio_set_pin_state( OUTPUTS, PUMPS, PUMP_REFILL, refill_state );
}
