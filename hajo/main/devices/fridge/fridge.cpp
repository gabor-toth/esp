#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "fridge.h"
#include <stdatomic.h>

static const char *TAG = "fridge";

#define FAN_1_OUT_POWER GPIO_NUM_1
#define FAN_1_OUT_PWM   GPIO_NUM_2
#define FAN_1_IN_SENSE  GPIO_NUM_3

#define PCNT_HIGH_LIMIT    (30000)
#define ANIMATION_SECS   10

static pcnt_unit_handle_t unit_handle = nullptr;
static int previous_counter[3] = { 0, 0, 0};
double duty_cycle;
bool ascending;
int animation_counter;

static void timer_callback( void *arg ) {
    int i = 0;
    int current_counter = 0;
    pcnt_unit_get_count( unit_handle, &current_counter );
    int count;
    if ( current_counter >= previous_counter[ i ] ) {
        count = current_counter - previous_counter[ i ];
    } else {
        count = PCNT_HIGH_LIMIT - previous_counter[ i ] + current_counter;
    }
    previous_counter[i] = current_counter;

    if (--animation_counter == 0) {
        animation_counter = ANIMATION_SECS;
        duty_cycle += 0.05 * ( ascending ? 1 : -1);
        if ( duty_cycle >= 1.0 ) {
            ascending = false;
            duty_cycle = 1.0;
        } else if ( duty_cycle <= 0.0 ) {
            ascending = true;
            duty_cycle = 0.0;
        }
    }

    // Set duty to 50%. (2 ** 13) * 50% = 4096
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                                  (1<<LEDC_TIMER_13_BIT) * duty_cycle ));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    ESP_LOGI(TAG, "Count %d, RPM %d, duty %0.2lf", count, count*60/2, duty_cycle);
}

static void setup_pcnt() {
    // see
    // - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html#
    // https://github.com/espressif/esp-idf/blob/v4.2.2/examples/peripherals/pcnt/main/pcnt_example_main.c

    gpio_config_t io_conf = {};

//    io_conf.intr_type = GPIO_INTR_DISABLE;
//    io_conf.mode = GPIO_MODE_INPUT;
//    io_conf.pin_bit_mask = BIT( FAN_1_IN_SENSE ) /* | BIT( FAN_2_IN_SENSE ) | BIT( FAN_3_IN_SENSE )*/;
//    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
//    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
//    gpio_config( &io_conf );

    gpio_set_level( FAN_1_OUT_POWER, 1   );
    gpio_set_level( FAN_1_OUT_PWM, 1 ); // PWM is inverted
    // ...

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( FAN_1_OUT_POWER ) | BIT( FAN_1_OUT_PWM ) /*|...*/ ;
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
    ESP_ERROR_CHECK(pcnt_new_unit( &unit_config, &unit_handle) );

    pcnt_glitch_filter_config_t filer_config = {
            .max_glitch_ns = 10000, // 10us
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit_handle, &filer_config));

    pcnt_chan_config_t channel_config = {
            .edge_gpio_num = FAN_1_IN_SENSE,
            .level_gpio_num= GPIO_NUM_NC,
            .flags{
                    .invert_edge_input = true,
                    .invert_level_input = false,
                    .virt_edge_io_level = 0,
                    .virt_level_io_level = 1,
                    .io_loop_back = false,
            }
    };
    pcnt_channel_handle_t channel_handle = nullptr;
    ESP_ERROR_CHECK( pcnt_new_channel( unit_handle, &channel_config, &channel_handle));
    ESP_ERROR_CHECK( pcnt_channel_set_edge_action( channel_handle,PCNT_CHANNEL_EDGE_ACTION_INCREASE,PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK( pcnt_channel_set_level_action( channel_handle,PCNT_CHANNEL_LEVEL_ACTION_KEEP,PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    ESP_ERROR_CHECK(pcnt_unit_enable( unit_handle));
    ESP_ERROR_CHECK(pcnt_unit_start( unit_handle));
}

static void setup_timer() {
    esp_timer_create_args_t tca = {
            .callback = (esp_timer_cb_t) timer_callback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = nullptr,
            .skip_unhandled_events = false
    };

    esp_timer_handle_t timer = nullptr;
    esp_err_t stat = esp_timer_create( &tca, &timer );
    if (stat != ESP_OK) {
        ESP_LOGE(TAG,"Failed to create timer, err 0x%x\n",stat);
        return;
    }
    esp_timer_start_periodic(timer, 1000000L);
}

static void setup_pwm()
{
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
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
            .gpio_num       = FAN_1_OUT_PWM,
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = LEDC_TIMER_0,
            .duty           = 0, // Set duty to 0%
            .hpoint         = 0,
            .flags{
                    .output_invert = 1
            }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

//    // Set duty to 50%. (2 ** 13) * 50% = 4096
//    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
//                    (1<<LEDC_TIMER_13_BIT) * 1.0 ));
//    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    duty_cycle = 0.0;
    ascending = true;
    animation_counter = ANIMATION_SECS;
}
void fridge_main() {
    setup_pcnt();
    setup_timer();
    setup_pwm();
}
