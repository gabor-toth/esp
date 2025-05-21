#include "driver/gpio.h"
#include "ds18b20.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "fridge.h"
#include "fridge_config.h"
#include "owb_gpio.h"

static const char *TAG = "fridge";

#define NUMBER_OF_DEVICES   1

static const gpio_num_t gpios_data[NUMBER_OF_DEVICES] = {
        TEMP_1_IN,
#if NUMBER_OF_DEVICES >= 2
        TEMP_2_IN,
#endif
#if NUMBER_OF_DEVICES >= 3
        TEMP_3_IN,
#endif
#if NUMBER_OF_DEVICES >= 4
        TEMP_4_IN,
#endif
};

static float corrections[NUMBER_OF_DEVICES] = {
        0.44,
#if NUMBER_OF_DEVICES >= 2
        -0.44,
#endif
#if NUMBER_OF_DEVICES >= 3
        0.0,
#endif
#if NUMBER_OF_DEVICES >= 4
        0.0,
#endif
};

static owb_gpio_driver_info driver_info[NUMBER_OF_DEVICES];
static OneWireBus *buses[NUMBER_OF_DEVICES];
static DS18B20_Info *devices[NUMBER_OF_DEVICES];

void fridge_temp_timer_handler() {
    for ( int i = 0; i < NUMBER_OF_DEVICES; i++ ) {
        DS18B20_Info *device = devices[ i ];
        ds18b20_wait_for_conversion( device );
        float temp;
        DS18B20_ERROR result = ds18b20_read_temp( device, &temp );
        if ( result != DS18B20_OK ) {
            ESP_LOGW( TAG, "temp %d: error %d", i, result );
        } else {
            temp += corrections[ i ];
            ESP_LOGI( TAG, "temp %d: %.1f° (%f°)", i, temp, temp );
            ds18b20_convert( device );
        }
    }
}

static void write_correction_to_device( int i, const DS18B20_Info *device ) {
    uint8_t trigger_high = 0;
    uint8_t trigger_low = 0;
    trigger_high = (int) ( corrections[ i ] * 100 );
    ds18b20_write_trigger( device, trigger_high, trigger_low );
}

static void setup_devices() {
    for ( int i = 0; i < NUMBER_OF_DEVICES; i++ ) {
        OneWireBus *bus = buses[ i ] = owb_gpio_initialize( driver_info + i, gpios_data[ i ] );
        DS18B20_Info *device = devices[ i ] = ds18b20_malloc();
        ds18b20_init_solo( device, bus );
        ds18b20_set_resolution( device, DS18B20_RESOLUTION_12_BIT );

        //write_correction_to_device( i, device );
        uint8_t trigger_high = 0;
        uint8_t trigger_low = 0;
        ds18b20_read_trigger( device, &trigger_high, &trigger_low );
        corrections[ i ] = (float) ( ( *(int8_t *) &trigger_high ) / 100.0 );
        ESP_LOGI( TAG, "index %d high %02x low %02x corr %.2f", i, trigger_high, trigger_low, corrections[ i ] );

        ds18b20_convert( device );
    }
}

void fridge_temp_setup() {
    setup_devices();

//    SetN2kTemperature();
}
