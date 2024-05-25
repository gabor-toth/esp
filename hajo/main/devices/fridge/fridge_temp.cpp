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

#define TEMP_1_IN  GPIO_NUM_10
#define TEMP_2_IN  GPIO_NUM_11
#define TEMP_3_IN  GPIO_NUM_12
#define TEMP_4_IN  GPIO_NUM_13

static const char *TAG = "fridge";

static const gpio_num_t gpios_data[4] = { TEMP_1_IN, TEMP_2_IN, TEMP_3_IN, TEMP_4_IN };

void fridge_temp_timer_handler() {
    for ( int i = 0; i < 4; i++ ) {
//        ESP_LOGI(TAG, "Count %d, RPM %d, duty %0.2lf", count, count*60/2, duty_cycle);
    }
}

static void setup_pcnt() {
}

void fridge_temp_setup() {
    setup_pcnt();
}
