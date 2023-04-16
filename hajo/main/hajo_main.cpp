#include "config.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "hajo_battery.h"
#include "hajo_display.h"
#include "hajo_fluid.h"
#include "lib/nvs_main.h"
#include "n2k_sender.h"

#define ESP32_CAN_TX_PIN N2K_GPIO_NUM_TX
#define ESP32_CAN_RX_PIN N2K_GPIO_NUM_RX
#define ESP32_CAN_STANDBY_PIN N2K_GPIO_NUM_STANDBY

#include "NMEA2000_CAN.h"

static const char *LOG = "hajo_main";

// defined by pins 26/21
#define DEVICE_TYPE_GAUGE_DISPLAY 0b111
#define DEVICE_TYPE_BATTERY_MONITOR 0b110
#define DEVICE_TYPE_RESERVED_1 0b01
#define DEVICE_TYPE_RESERVED_0 0b00

static int device_type = 0xff;

static void determine_device_type() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
            ( 1 << GPIO_NUM_DEVICE_TYPE_0 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_1 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_2 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config( &io_conf );
    vTaskDelay(pdMS_TO_TICKS( 10 ));

    device_type = ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_2 ) << 2 ) |
                  ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_1 ) << 1 ) |
                  gpio_get_level( GPIO_NUM_DEVICE_TYPE_0 );
    ESP_LOGI( LOG, "device type %d", device_type );

    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

static void initialize_twai_driver() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = BIT( N2K_GPIO_NUM_STANDBY );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
    gpio_set_level( N2K_GPIO_NUM_STANDBY, 0 );
}

void hajo_main() {
    nvs_init();
    determine_device_type();
    initialize_twai_driver();

//    esp_pm_config_esp32s2_t pm_config = {
//            .max_freq_mhz = 80,
//            .min_freq_mhz = 40,
//            .light_sleep_enable = false
//    };
//
//    ESP_ERROR_CHECK( esp_pm_configure( &pm_config ));
    esp_pm_config_esp32s2_t pm_config;
    ESP_ERROR_CHECK( esp_pm_get_configuration( &pm_config ));
    ESP_LOGI( LOG, "Clock min: %d max: %d", pm_config.min_freq_mhz, pm_config.max_freq_mhz );

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    int iDev = 0;
    switch ( device_type ) {
        case DEVICE_TYPE_GAUGE_DISPLAY:
            hajo_fluid_main( iDev++ );
            hajo_display_main( iDev++ );
            break;
        case DEVICE_TYPE_BATTERY_MONITOR:
            hajo_battery_main( iDev++ );
            break;
        default:
            // TODO fail
            ESP_LOGE( LOG, "Unhandled device type %c%c%c",
                      device_type & 4 ? '1' : '0',
                      device_type & 2 ? '1' : '0',
                      device_type & 1 ? '1' : '0' );
            break;
    }
    n2k_open();
}

extern "C" {
void app_main() {
    hajo_main();
}
}
