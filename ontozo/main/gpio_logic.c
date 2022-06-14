#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "config.h"
#include "lib/gpio_define.h"
#include "lib/nvs_main.h"
#include "gpio_json.h"
#include "gpio_logic.h"

static const char *LOG_TAG = "logic";

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

static char *const PUMPS_NAME = "pumps";
static char *const ZONES_NAME = "zones";
static char *const BUTTON_NAME = "button";

static char *const LEVEL_NAME = "level";

static void set_initial_pump_states() {
    int level1 = gpio_get_pin_state( INPUTS, LEVELS_CLASS, 0 );
    int level2 = gpio_get_pin_state( INPUTS, LEVELS_CLASS, 1 );
    int level3 = gpio_get_pin_state( INPUTS, LEVELS_CLASS, 2 );
    int level4 = gpio_get_pin_state( INPUTS, LEVELS_CLASS, 3 );

    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_MAIN, false );
    // reached( level0 ) && reached( level1 )
    bool refillState = !reached( level4 )
                       || !reached( level3 )
                       || !reached( level2 )
                       || !reached( level1 );
    ESP_LOGI( LOG_TAG, "Levels: 1-%d 2-%d 3-%d 4-%d refillPump-%d", level1, level2, level3, level4, refillState );
    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_REFILL,
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

char *read_nvs_or_default( nvs_handle_t nvs_handle, const char *prefix, int index ) {
    char nvs_key[256];
    char nvs_default[256];

    snprintf( nvs_key, sizeof nvs_key, "%s.%d.name", prefix, index );
    char *string_read = nvs_read_string( nvs_handle, nvs_key );
    if ( string_read != NULL) {
        return string_read;
    }

    snprintf( nvs_default, sizeof nvs_key, "%s%d", prefix, index );
    return strdup( nvs_default );
}

void gpio_define_output_pins_callback( gpio_config_t *io_conf, void *user_context ) {
    nvs_handle_t nvs_handle = (nvs_handle_t) user_context;

    gpio_add_class( OUTPUTS, PUMPS_NAME, 2, high_is_on );
    gpio_add_class( OUTPUTS, ZONES_NAME, 8, low_is_on );

    int index;
    for ( int i = 0; i < sizeof pump_pins / sizeof pump_pins[ 0 ]; i++ ) {
        index = gpio_add_pin_with_allocated_name( OUTPUTS, PUMPS_CLASS, pump_pins[ i ],
                                                  read_nvs_or_default( nvs_handle, PUMPS_NAME, i + 1 ),
                                                  inherit,
                                                  &io_conf->pin_bit_mask );

    }
    for ( int i = 0; i < sizeof zone_pins / sizeof zone_pins[ 0 ]; i++ ) {
        gpio_add_pin_with_allocated_name( OUTPUTS, ZONES_CLASS, zone_pins[ i ],
                                          read_nvs_or_default( nvs_handle, ZONES_NAME, i + 1 ),
                                          inherit,
                                          &io_conf->pin_bit_mask );
    }

    set_initial_pump_states();
}

void gpio_define_input_pins_callback( gpio_config_t *io_conf, void *user_context ) {
    nvs_handle_t nvs_handle = (nvs_handle_t) user_context;

    gpio_add_class( INPUTS, "levels", 4, low_is_on );
    gpio_add_class( INPUTS, "buttons", 2, low_is_on );

    int index;
    gpio_add_pin_with_allocated_name( INPUTS, LEVELS_CLASS, GPIO_INPUT_LEVEL_1,
                                      read_nvs_or_default( nvs_handle, LEVEL_NAME, 1 ),
                                      low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin_with_allocated_name( INPUTS, LEVELS_CLASS, GPIO_INPUT_LEVEL_2,
                                      read_nvs_or_default( nvs_handle, LEVEL_NAME, 2 ),
                                      low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin_with_allocated_name( INPUTS, LEVELS_CLASS, GPIO_INPUT_LEVEL_3,
                                      read_nvs_or_default( nvs_handle, LEVEL_NAME, 3 ),
                                      high_is_on, &io_conf->pin_bit_mask );
    index = gpio_add_pin_with_allocated_name( INPUTS, LEVELS_CLASS, GPIO_INPUT_LEVEL_4,
                                              read_nvs_or_default( nvs_handle, LEVEL_NAME, 4 ),
                                              high_is_on, &io_conf->pin_bit_mask );
    gpio_set_delays( INPUTS, LEVELS_CLASS, index, 10 * 1000, 10 * 1000 );
    gpio_add_pin_with_allocated_name( INPUTS, BUTTONS_CLASS, GPIO_INPUT_BUTTON_START,
                                      read_nvs_or_default( nvs_handle, BUTTON_NAME, 1 ),
                                      low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin_with_allocated_name( INPUTS, BUTTONS_CLASS, GPIO_INPUT_BUTTON_STOP,
                                      read_nvs_or_default( nvs_handle, BUTTON_NAME, 2 ),
                                      low_is_on, &io_conf->pin_bit_mask );
}

void gpio_changed_callback( uint32_t io_num, int state ) {
    ESP_LOGI( LOG_TAG, "Pin %d changed to %d\n", io_num, state );

    bool refill_state = gpio_get_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_REFILL );
    bool main_state = gpio_get_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_MAIN );

    if ( io_num == GPIO_INPUT_LEVEL_4 ) {
        if ( state == PIN_ENABLED ) {
            refill_state = false;
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

    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_MAIN, main_state );
    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_REFILL, refill_state );
}

static void level4_drop_timeout( TimerHandle_t timer ) {
    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_REFILL, true );
}

void gpio_logic_init() {
    nvs_handle_t nvs_handle = nvs_open_storage();
    gpio_init((void *) nvs_handle );
    nvs_close_storage( nvs_handle );
}

void gpio_pump_main( bool on ) {
    gpio_set_pin_state( OUTPUTS, PUMPS_CLASS, PUMP_MAIN, on );
}