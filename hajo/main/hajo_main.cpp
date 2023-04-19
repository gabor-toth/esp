#include "config.h"
#include "driver/gpio.h"
#include "esp_private/esp_clk.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "hajo_battery.h"
#include "hajo_display.h"
#include "hajo_fluid.h"
#include "hajo_logger.h"
#include "lib/nvs_main.h"
#include "n2k_sender.h"

#define ESP32_CAN_TX_PIN N2K_GPIO_NUM_TX
#define ESP32_CAN_RX_PIN N2K_GPIO_NUM_RX
#define ESP32_CAN_STANDBY_PIN N2K_GPIO_NUM_STANDBY

#include "NMEA2000_CAN.h"

static const char *LOG = "hajo_main";

// defined by pins 10-12
#define DEVICE_TYPE_LOGGER 0b011
#define DEVICE_TYPE_GAUGE_DISPLAY 0b101
#define DEVICE_TYPE_BATTERY_MONITOR 0b110

static int device_type = 0xff;

static void determine_device_type() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
            BIT( GPIO_NUM_DEVICE_TYPE_0 ) |
            BIT( GPIO_NUM_DEVICE_TYPE_1 ) |
            BIT( GPIO_NUM_DEVICE_TYPE_2 );
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

esp_pm_lock_handle_t pm_lock_handle_display;
esp_pm_lock_handle_t pm_lock_handle_listen;

static void clock_configure( int max_freq_mhz ) {

    static esp_pm_config_esp32s2_t pm_config = {
            .max_freq_mhz = max_freq_mhz,
            .min_freq_mhz = 80,
            .light_sleep_enable = false
    };

    ESP_ERROR_CHECK( esp_pm_configure( &pm_config ));
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_APB_FREQ_MAX, 0, "listen mode", &pm_lock_handle_listen ));
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_CPU_FREQ_MAX, 0, "display mode", &pm_lock_handle_display ));
    ESP_ERROR_CHECK( esp_pm_lock_acquire( pm_lock_handle_listen ));
}

static void clock_log_state() {
    esp_pm_config_esp32s2_t pm_config;
    ESP_ERROR_CHECK( esp_pm_get_configuration( &pm_config ));
    int cpu_freq = esp_clk_cpu_freq();
    ESP_LOGI( LOG, "Clock min: %d max: %d current: %d",
              pm_config.min_freq_mhz,
              pm_config.max_freq_mhz,
              cpu_freq / 1000000 );
}

void hajo_main() {
    nvs_init();
    determine_device_type();
    initialize_twai_driver();

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    int iDev = 0;
    switch ( device_type ) {
        case DEVICE_TYPE_GAUGE_DISPLAY:
            clock_configure( 240 );
            hajo_fluid_main( iDev++ );
            hajo_display_main( iDev++ );
            break;
        case DEVICE_TYPE_BATTERY_MONITOR:
            clock_configure( 80 );
            hajo_battery_main( iDev++ );
            break;
        case DEVICE_TYPE_LOGGER:
            hajo_logger_main();
            break;
        default:
            // TODO fail
            ESP_LOGE( LOG, "Unhandled device type %c%c%c",
                      device_type & 4 ? '1' : '0',
                      device_type & 2 ? '1' : '0',
                      device_type & 1 ? '1' : '0' );
            break;
    }

    clock_log_state();

    n2k_open();
}

extern "C" {
void app_main() {
    hajo_main();
}
}
