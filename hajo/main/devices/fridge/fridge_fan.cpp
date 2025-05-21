#include <climits>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "fridge.h"
#include "fridge_config.h"

static const char *TAG = "fridge";

#define NUMBER_OF_DEVICES   1

#define PCNT_HIGH_LIMIT    (30000)

static const gpio_num_t gpios_power[NUMBER_OF_DEVICES] = {
        FAN_1_OUT_POWER,
#if NUMBER_OF_DEVICES >= 2
        FAN_2_OUT_POWER,
#endif
#if NUMBER_OF_DEVICES >= 3
        FAN_3_OUT_POWER,
#endif
};
static const gpio_num_t gpios_pwm[NUMBER_OF_DEVICES] = {
        FAN_1_OUT_PWM,
#if NUMBER_OF_DEVICES >= 2
        FAN_2_OUT_PWM,
#endif
#if NUMBER_OF_DEVICES >= 3
        FAN_3_OUT_PWM,
#endif
};
static const gpio_num_t gpios_sense[NUMBER_OF_DEVICES] = {
        FAN_1_IN_SENSE,
#if NUMBER_OF_DEVICES >= 2
        FAN_2_IN_SENSE,
#endif
#if NUMBER_OF_DEVICES >= 3
        FAN_3_IN_SENSE,
#endif
};

static pcnt_unit_handle_t unit_handles[NUMBER_OF_DEVICES] = {
        nullptr,
#if NUMBER_OF_DEVICES >= 2
        nullptr,
#endif
#if NUMBER_OF_DEVICES >= 3
        nullptr,
#endif
};
static int previous_counter[NUMBER_OF_DEVICES] = {
        0,
#if NUMBER_OF_DEVICES >= 2
        0,
#endif
#if NUMBER_OF_DEVICES >= 3
        0,
#endif
};

void fridge_fan_set_duty_cycle( int index, double _duty_cycle ) {
    auto channel = (ledc_channel_t) index;
    // Set duty to 50%. (2 ** 13) * 50% = 4096
    ESP_ERROR_CHECK( ledc_set_duty( LEDC_LOW_SPEED_MODE, channel,
                                    ( 1 << LEDC_TIMER_13_BIT ) * _duty_cycle ) );
    ESP_ERROR_CHECK( ledc_update_duty( LEDC_LOW_SPEED_MODE, channel ) );
}

void fridge_fan_timer_handler() {
    for ( int i = 0; i < NUMBER_OF_DEVICES; i++ ) {
        int current_counter = 0;
        pcnt_unit_get_count( unit_handles[ i ], &current_counter );
        int count;
        if ( current_counter >= previous_counter[ i ] ) {
            count = current_counter - previous_counter[ i ];
        } else {
            count = PCNT_HIGH_LIMIT - previous_counter[ i ] + current_counter;
        }
        previous_counter[ i ] = current_counter;
        ESP_LOGI( TAG, "Channel %d count %d RPM %d", i, count, count * 60 / 2 );
    }
}

static void setup_fridge_pcnt() {
    // see
    // - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html#
    // - https://github.com/espressif/esp-idf/blob/v4.2.2/examples/peripherals/pcnt/main/pcnt_example_main.c

    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = 0;
    for ( auto gpio: gpios_power ) {
        gpio_set_level( gpio, 1 );
        io_conf.pin_bit_mask |= BIT( gpio );
    }

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );

    pcnt_unit_config_t unit_config = {
            .low_limit = SHRT_MIN,
            .high_limit = PCNT_HIGH_LIMIT,
            .intr_priority = 0,
            .flags = {
                    .accum_count = false,
            }
    };
    pcnt_chan_config_t channel_config = {
            .edge_gpio_num = GPIO_NUM_NC,
            .level_gpio_num= GPIO_NUM_NC,
            .flags{
                    .invert_edge_input = true,
                    .invert_level_input = false,
                    .virt_edge_io_level = 0,
                    .virt_level_io_level = 1,
                    .io_loop_back = false,
            }
    };

    for ( int i = 0; i < NUMBER_OF_DEVICES; i++ ) {
        pcnt_unit_handle_t unit_handle = nullptr;
        ESP_ERROR_CHECK( pcnt_new_unit( &unit_config, &unit_handle ) );
        unit_handles[ i ] = unit_handle;

        pcnt_glitch_filter_config_t filer_config = {
                .max_glitch_ns = 10000, // 10us
        };
        ESP_ERROR_CHECK( pcnt_unit_set_glitch_filter( unit_handle, &filer_config ) );

        pcnt_channel_handle_t channel_handle = nullptr;
        channel_config.edge_gpio_num = gpios_sense[ i ];
        ESP_ERROR_CHECK( pcnt_new_channel( unit_handle, &channel_config, &channel_handle ) );
        ESP_ERROR_CHECK( pcnt_channel_set_edge_action( channel_handle, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                       PCNT_CHANNEL_EDGE_ACTION_HOLD ) );
        ESP_ERROR_CHECK( pcnt_channel_set_level_action( channel_handle, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                        PCNT_CHANNEL_LEVEL_ACTION_KEEP ) );

        ESP_ERROR_CHECK( pcnt_unit_enable( unit_handle ) );
        ESP_ERROR_CHECK( pcnt_unit_start( unit_handle ) );
    }

}

static void setup_fridge_pwm() {
    // see
    // - https://github.com/espressif/esp-idf/blob/v5.2.1/examples/peripherals/ledc/ledc_basic/main/ledc_basic_example_main.c
    // - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html#_CPPv411ledc_mode_t

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .duty_resolution  = LEDC_TIMER_13_BIT,
            .timer_num        = LEDC_TIMER_0,
            .freq_hz          = 4096,
            .clk_cfg          = LEDC_AUTO_CLK,
            .deconfigure = false
    };
    ESP_ERROR_CHECK( ledc_timer_config( &ledc_timer ) );

    ledc_channel_config_t ledc_channel = {
            .gpio_num       = GPIO_NUM_NC,
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = LEDC_TIMER_0,
            .duty           = 0, // Set duty to 0%
            .hpoint         = 0,
            .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
            .flags{
                    .output_invert = 0
            }
    };

    for ( int i = 0; i < NUMBER_OF_DEVICES; i++ ) {
        // Prepare and then apply the LEDC PWM channel configuration
        ledc_channel.gpio_num = gpios_pwm[ i ];
        ledc_channel.channel = static_cast<ledc_channel_t>(LEDC_CHANNEL_0 + i);
        ESP_ERROR_CHECK( ledc_channel_config( &ledc_channel ) );
    }
}

void fridge_fan_setup() {
    setup_fridge_pcnt();
    setup_fridge_pwm();
}
