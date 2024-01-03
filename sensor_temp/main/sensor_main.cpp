#include "config.h"
#include "ds18b20.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_main.h"
#include "owb_gpio.h"

static const char *TAG = "sensor_main";

void sensor_main() {
    nvs_init();

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    owb_gpio_driver_info driver_info;
    OneWireBus* bus = owb_gpio_initialize(&driver_info, GPIO_NUM_14 );
    DS18B20_Info* device = ds18b20_malloc();
    ds18b20_init_solo(device, bus);
    ds18b20_set_resolution(device, DS18B20_RESOLUTION_12_BIT );
    if ( ds18b20_convert(device) ) {
        ds18b20_wait_for_conversion(device);
        float temp;
        DS18B20_ERROR result = ds18b20_read_temp(device, &temp);
        ESP_LOGI(TAG, "result %d %f", result, temp);
    }
}

extern "C" {
void app_main() {
    sensor_main();
}
}
