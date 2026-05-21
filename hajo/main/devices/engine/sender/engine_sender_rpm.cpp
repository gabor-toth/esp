#include "driver/pulse_cnt.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "engine_sender_internal.h"
#include "freertos/FreeRTOS.h"
#include "gpio_define.h"
#include "n2k/n2k_receiver.h"

static const char *TAG = "rpm";

static pcnt_unit_handle_t pcnt_unit = nullptr;
static double *engineSpeed;
static int ticksPerRevolution;
static int rpm;

static bool pulse_counter_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx) {
    pcnt_unit_clear_count(pcnt_unit);
    rpm++;
    return false;
}

void setup_pulse_counter() {
    // see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html#application-examples

    pcnt_unit_config_t unit_config = {
        .low_limit = SHRT_MIN,
        .high_limit = SHRT_MAX,
        .intr_priority = 1
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 300,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PIN_INPUT_RPM_SENSOR,
        .level_gpio_num = GPIO_NUM_NC,
    };
    pcnt_channel_handle_t pcnt_chan_a = nullptr;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    ESP_ERROR_CHECK(
        pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, ticksPerRevolution));
    pcnt_event_callbacks_t callbacks = {
        .on_reach = pulse_counter_on_reach,
    };
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &callbacks, queue));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

static bool timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    *engineSpeed = rpm;
    rpm = 0;
    return false;
}

static void setup_timer() {
    // see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gptimer.html#triggering-periodic-alarm-events

    gptimer_handle_t gptimer = nullptr;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP, // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000, // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };
    // Create a timer instance
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
        .reload_count = 0, // When the alarm event occurs, the timer will automatically reload to 0
        .flags{
            .auto_reload_on_alarm = true, // Enable auto-reload function
        }
    };
    // Set the timer's alarm action
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_callback, // Call the user callback function when the alarm event occurs
    };
    // Register timer event callback functions, allowing user context to be carried
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &callbacks, nullptr));
    // Enable the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void setup_rpm(int _ticks_per_revolution, double *_engineSpeed) {
    ticksPerRevolution = _ticks_per_revolution;
    engineSpeed = _engineSpeed;
    setup_pulse_counter();
    setup_timer();
}
