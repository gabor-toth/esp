#include "config.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "devices/battery/hajo_battery.h"
#include "devices/display/hajo_display.h"
#include "devices/fridge/fridge.h"
#include "devices/fluid/hajo_fluid.h"
#include "devices/rudder/hajo_rudder.h"
#include "devices/signalk/hajo_signalk.h"
#include "devices/signalk/EspSigK.h"
#include "devices/witmotion//wit_main.h"
#include "nvs_main.h"
#include "n2k/n2k_receiver.h"
#include "nodeinfo/nodeinfo.h"

#define LED_TIME_ON 20
#define LED_TIME_GAP 200
#define LED_TIME_INTERVAL 3000

#define ESP32_CAN_TX_PIN N2K_GPIO_NUM_TX
#define ESP32_CAN_RX_PIN N2K_GPIO_NUM_RX
#define ESP32_CAN_STANDBY_PIN N2K_GPIO_NUM_STANDBY

#include "NMEA2000_CAN.h"

#define TO_3_BITS(X)    ((X)&4?'1':'0'),((X)&2?'1':'0'),((X)&1?'1':'0')

static const char *LOG = "hajo_main";

static int hardware_device_type = 0xff;

static const char *device_type_names[] = {
        "unused 0",         //  000
        "n2k gateway",      //  001
        "unused 2",         //  010
        "unused 3",         //  011
        "rudder",           //  100
        "fluid & display",  //  101
        "battery monitor",  //  110
        "unused 7",         //  111
        "DEVICE_TYPE_ALL",  // 1000
};

static bool determine_device_type( int firmware_device_type ) {
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
    vTaskDelay( pdMS_TO_TICKS( 10 ) );

    hardware_device_type = ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_2 ) << 2 ) |
                           ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_1 ) << 1 ) |
                           gpio_get_level( GPIO_NUM_DEVICE_TYPE_0 );
    hardware_device_type &= 0b111;
    if ( DEVICE_TYPE_ALL == firmware_device_type ) {
        ESP_LOGE( LOG, "No firmware device type specified (DEVICE_TYPE_ALL), hardware is \"%s\" (%c%c%c), check the end of config.h",
                  device_type_names[ hardware_device_type ],
                  TO_3_BITS(hardware_device_type) );
    }
    if ( hardware_device_type != firmware_device_type ) {
        ESP_LOGE( LOG, "Firmware \"%s\" (%c%c%c) does not match hardware \"%s\" (%c%c%c)",
                  device_type_names[ firmware_device_type ],
                  TO_3_BITS(firmware_device_type),
                  device_type_names[ hardware_device_type ],
                  TO_3_BITS(hardware_device_type) );
        //ESP_ERROR_CHECK( ESP_ERR_NOT_SUPPORTED );
        return false;
    }
    ESP_LOGI( LOG, "device type %d %s", hardware_device_type, device_type_names[ hardware_device_type ] );

    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
    return true;
}

static void led_on() {
    gpio_set_direction( GPIO_NUM_LED_POWER, GPIO_MODE_OUTPUT );
    gpio_set_level( GPIO_NUM_LED_POWER, 1 );
}

_Noreturn static void task_power_led( void *arg ) {
    (void) arg;

    TickType_t flashMarker = 0;
    for ( ;; ) {
        gpio_set_level( GPIO_NUM_LED_POWER, 1 );
        vTaskDelayUntil( &flashMarker, pdMS_TO_TICKS( LED_TIME_ON ) );
        gpio_set_level( GPIO_NUM_LED_POWER, 0 );
        vTaskDelayUntil( &flashMarker, pdMS_TO_TICKS( LED_TIME_GAP ) );
        gpio_set_level( GPIO_NUM_LED_POWER, 1 );
        vTaskDelayUntil( &flashMarker, pdMS_TO_TICKS( LED_TIME_ON ) );
        gpio_set_level( GPIO_NUM_LED_POWER, 0 );
        vTaskDelayUntil( &flashMarker, pdMS_TO_TICKS( LED_TIME_INTERVAL - 2 * LED_TIME_ON - LED_TIME_GAP ) );
    }
}

static void initialize_twai_driver() {
#if N2K_GPIO_NUM_STANDBY != GPIO_NUM_NC
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = BIT( N2K_GPIO_NUM_STANDBY );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
    gpio_set_level( N2K_GPIO_NUM_STANDBY, 0 );
#endif
}

esp_pm_lock_handle_t pm_lock_handle_display;
esp_pm_lock_handle_t pm_lock_handle_listen;

static void clock_configure( int max_freq_mhz ) {

    static esp_pm_config_t pm_config = {
            .max_freq_mhz = max_freq_mhz,
            .min_freq_mhz = 80,
            .light_sleep_enable = false
    };

    ESP_ERROR_CHECK( esp_pm_configure( &pm_config ) );
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_APB_FREQ_MAX, 0, "listen mode", &pm_lock_handle_listen ) );
    ESP_ERROR_CHECK( esp_pm_lock_create( ESP_PM_CPU_FREQ_MAX, 0, "display mode", &pm_lock_handle_display ) );
    ESP_ERROR_CHECK( esp_pm_lock_acquire( pm_lock_handle_listen ) );
}

static int own_log_vprintf( const char *format, va_list args ) {
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    TaskStatus_t taskStatus;
    vTaskGetInfo( task, &taskStatus, pdFALSE, eRunning );
    printf( LOG_COLOR_I "%s ", taskStatus.pcTaskName );
    return vprintf( format, args );
}

void hajo_main() {
    //debug_start_task_dump();

    esp_log_set_vprintf( own_log_vprintf );

    led_on();
    xTaskCreate( task_power_led, "power_led", 1024, nullptr, 10, nullptr );

    nvs_init();
    if ( !determine_device_type( DEVICE_TYPE ) ) {
        return;
    }
    initialize_twai_driver();

    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    int iDev = 0;
#if DEVICE_TYPE == DEVICE_TYPE_BATTERY || DEVICE_TYPE == DEVICE_TYPE_ALL
    clock_configure( 80 );
    hajo_battery_main( iDev++ );
#endif
#if DEVICE_TYPE == DEVICE_TYPE_DISPLAY || DEVICE_TYPE == DEVICE_TYPE_ALL
    clock_configure( 240 );
    NMEA2000.SetDeviceCount( 2 );
    hajo_fluid_main( iDev++ );
    hajo_display_main( iDev++ );
#endif
#if DEVICE_TYPE == DEVICE_TYPE_FRIDGE || DEVICE_TYPE == DEVICE_TYPE_ALL
    fridge_main();
#endif
#if DEVICE_TYPE == CONFIG_DEVICE_TYPE_GATEWAY || DEVICE_TYPE == DEVICE_TYPE_ALL
    clock_configure( 240 );
    NMEA2000.SetDeviceCount( 3 );
    wit_main( iDev++ );
    //hajo_attitude_main( iDev++ );
    hajo_signalk_main( iDev++ );
#endif
#if DEVICE_TYPE == DEVICE_TYPE_RUDDER || DEVICE_TYPE == DEVICE_TYPE_ALL
    clock_configure( 240 );
    NMEA2000.SetDeviceCount( 1 );
    hajo_rudder_main( iDev++ );
#endif

    node_info_main();
    n2k_init();
}

extern "C" {
void app_main() {
    hajo_main();
}
}
