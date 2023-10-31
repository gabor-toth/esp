#include "config.h"
#include "driver/gpio.h"
#include "esp_private/esp_clk.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#if DEVICE_TYPE == DEVICE_TYPE_GAUGE_DISPLAY
#include "devices/display/hajo_display.h"
#include "devices/display/hajo_fluid.h"
#elif DEVICE_TYPE == DEVICE_TYPE_BATTERY_MONITOR
#include "devices/battery/hajo_battery.h"
#elif DEVICE_TYPE == DEVICE_TYPE_LOGGER
#include "devices/logger/hajo_logger.h"
#endif
#include "lib/nvs_main.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"

#define ESP32_CAN_TX_PIN N2K_GPIO_NUM_TX
#define ESP32_CAN_RX_PIN N2K_GPIO_NUM_RX
#define ESP32_CAN_STANDBY_PIN N2K_GPIO_NUM_STANDBY

#include "NMEA2000_CAN.h"

static const char *LOG = "hajo_main";

static int hardware_device_type = 0xff;

static const char *device_type_names[] = {
        "unknown 0",
        "logger",
        "unknown 2",
        "unknown 3",
        "unknown 4",
        "display",
        "monitor",
        "unknown 7",
};

static void determine_device_type( int firmware_device_type ) {
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

    hardware_device_type = ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_2 ) << 2 ) |
                           ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_1 ) << 1 ) |
                           gpio_get_level( GPIO_NUM_DEVICE_TYPE_0 );
    hardware_device_type &= 0b111;
    if ( hardware_device_type != firmware_device_type ) {
        ESP_LOGE( LOG, "Firmware %s does not match hardware %s",
                  device_type_names[ firmware_device_type],
                  device_type_names[ hardware_device_type ]);
        ESP_ERROR_CHECK( ESP_ERR_NOT_SUPPORTED );
    }
    ESP_LOGI( LOG, "device type %d %s", hardware_device_type, device_type_names[ hardware_device_type ] );

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

    static esp_pm_config_t pm_config = {
            .max_freq_mhz = max_freq_mhz,
            .min_freq_mhz = 80,
            .light_sleep_enable = false
    };

    ESP_ERROR_CHECK( esp_pm_configure( &pm_config ));
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_APB_FREQ_MAX, 0, "listen mode", &pm_lock_handle_listen ));
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_CPU_FREQ_MAX, 0, "display mode", &pm_lock_handle_display ));
    ESP_ERROR_CHECK( esp_pm_lock_acquire( pm_lock_handle_listen ));
}

void hajo_main() {
    nvs_init();
    determine_device_type( DEVICE_TYPE );
    initialize_twai_driver();

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    int iDev = 0;
#if DEVICE_TYPE == DEVICE_TYPE_GAUGE_DISPLAY
    hajo_fluid_main( iDev++ );
    hajo_display_main( iDev++ );
    clock_configure( 240 );
#elif DEVICE_TYPE == DEVICE_TYPE_BATTERY_MONITOR
    hajo_battery_main( iDev++ );
    clock_configure( 80 );
#elif DEVICE_TYPE == DEVICE_TYPE_LOGGER
    hajo_logger_main( iDev++ );
    clock_configure( 240 );
#else
    #error Unhandled device type ## DEVICE_TYPE
#endif

    n2k_init();
}

extern "C" {
void app_main() {
    hajo_main();
}
}
