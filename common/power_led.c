#include "driver/gpio.h"
#include "power_led.h"
#include "esp_log.h"
#include "esp_task.h"

static const char* TAG = "power_led";

typedef struct
{
    int time_on;
    int time_gap;
    int group_count;
    int time_interval;
} led_config_t;

static led_config_t modes[] = {
    {.time_on = 20, .time_gap = 200, .group_count = 1, .time_interval = 0},
    {.time_on = 20, .time_gap = 200, .group_count = 2, .time_interval = 1000},
    {.time_on = 20, .time_gap = 0, .group_count = 1, .time_interval = 3000},
};

static led_config_t current_mode;
static led_config_t next_mode;

static void led_on()
{
    gpio_set_direction(GPIO_NUM_LED_POWER, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_LED_POWER, 1);
}

_Noreturn static void task_power_led(void* arg)
{
    (void)arg;

    TickType_t flashMarker = 0;
    int time_on = 0;
    int time_gap = 0;
    int group_count = 0;
    int time_interval = 0;
    for (;;)
    {
        if (next_mode.time_on != 0)
        {
            current_mode = next_mode;
            next_mode.time_on = 0;
            group_count = current_mode.group_count;
            time_on = current_mode.time_on;
            time_gap = current_mode.time_gap - current_mode.time_on;
            if (time_gap < 0)
            {
                time_gap = 0;
            }
            time_interval = current_mode.time_interval - 2 * time_on - time_gap;
            if (time_interval < 0)
            {
                time_interval = 0;
            }
            time_on = pdMS_TO_TICKS(time_on);
            time_gap = pdMS_TO_TICKS(time_gap);
            time_interval = pdMS_TO_TICKS(time_interval);
        }
        for (int i = 0; i < group_count; i++)
        {
            gpio_set_level(GPIO_NUM_LED_POWER, 1);
            vTaskDelayUntil(&flashMarker, time_on);
            gpio_set_level(GPIO_NUM_LED_POWER, 0);
            if (time_gap > 0)
            {
                vTaskDelayUntil(&flashMarker, time_gap);
            }
        }
        if (time_interval > 0)
        {
            vTaskDelayUntil(&flashMarker, time_interval);
        }
    }
}

void power_led_set_mode(int mode)
{
    if (mode < 0 || mode >= sizeof(modes) / sizeof(modes[0]))
    {
        ESP_LOGW(TAG, "Bad mode: %d", mode);
        return;
    }
    ESP_LOGI(TAG, "Setting mode: %d", mode);
    next_mode = modes[mode];
}

void power_led_main()
{
    led_on();
    next_mode = modes[0];
    xTaskCreate(task_power_led, "power_led", 3072, NULL, 10, NULL);
}
