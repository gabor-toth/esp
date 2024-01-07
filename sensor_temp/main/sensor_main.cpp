#include "config.h"
#include "ds18b20.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_main.h"
#include "owb_gpio.h"
#include "timer.h"
#include "wifi/wifi_main.h"

static const char *TAG = "sensor_main";
static owb_gpio_driver_info driver_info;
static OneWireBus *bus;
static DS18B20_Info *device;

#if CONFIG_DS18B20_RESOLUTION_BITS_12
#define DS18B20_RESOLUTION_BITS DS18B20_RESOLUTION_12_BIT
#elif CONFIG_DS18B20_RESOLUTION_BITS_11
#define DS18B20_RESOLUTION_BITS DS18B20_RESOLUTION_11_BIT
#elif CONFIG_DS18B20_RESOLUTION_BITS_10
#define DS18B20_RESOLUTION_BITS DS18B20_RESOLUTION_10_BIT
#elif CONFIG_DS18B20_RESOLUTION_BITS_9
#define DS18B20_RESOLUTION_BITS DS18B20_RESOLUTION_9_BIT
#else
#error Unknown DS18B20_RESOLUTION_BITS value
#endif

void tempsens_init() {
    ESP_LOGI( TAG, "tempsens_init start" );
    bus = owb_gpio_initialize( &driver_info, CONFIG_DS18B20_GPIO_NUM );
    device = ds18b20_malloc();
    ds18b20_init_solo( device, bus );
    ds18b20_set_resolution( device, DS18B20_RESOLUTION_BITS );
    ESP_LOGI( TAG, "tempsens_init finish" );
}

void tempsens_measure( void *user_data ) {
    (void) user_data;
    
    for ( ;; ) {
        if ( ds18b20_convert( device ) ) {
            ds18b20_wait_for_conversion( device );
            float temp;
            DS18B20_ERROR result = ds18b20_read_temp( device, &temp );
            if () {
                ESP_LOGW( TAG, "bad result %d, repeating read", result );
                continue;
            }
            if ( temp < -40.0 || temp > 100.0 ) {
                ESP_LOGW( TAG, "bad temperature %f, repeating read", temp );
                continue;
            }
            ESP_LOGI( TAG, "temperature %f", temp );
            break;
        }
    }
}

void sensor_main() {
    nvs_init();
    
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
    tempsens_init();
    timer_start( "measure", tempsens_measure, 1000, nullptr, true );
    ESP_ERROR_CHECK( wifi_connect() );
}

extern "C" {
void app_main() {
    sensor_main();
}
}
