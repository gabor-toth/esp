#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "hajo_adc.h"
#include "n2k_png.h"
#include "n2k_protocol.h"
#include "n2k_sender.h"
#include "lib/main_main.h"
#include "lib/nvs_main.h"

static const char *LOG = "hajo_main";

// defined by pins 26/21
#define DEVICE_TYPE_GAUGE_DISPLAY 0b11
#define DEVICE_TYPE_BATTERY_MONITOR 0b10
#define DEVICE_TYPE_RESERVED_1 0b01
#define DEVICE_TYPE_RESERVED_0 0b00

static int device_type = 0xff;

static void set_pins() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
            BIT1 | BIT2 | BIT3;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = true;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
            // ADC in BIT0 | BIT1 | BIT3 |
            // TWAI BIT4 | BIT5 |
            BIT6 | BIT7 | BIT8 | BIT9 |
            BIT10 | BIT11 | BIT12 | BIT13 | BIT14 |
            BIT15 | BIT16 | BIT17 | BIT18 | BIT19 |
            BIT20 | BIT21 | BIT26 |
            BIT33 | BIT34 | BIT35 | BIT36 | BIT37 | BIT38 | BIT39 |
            BIT40 | BIT41 | BIT42 |
            // USB UART BIT43 | BIT44 |
            BIT45;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config( &io_conf );

//    io_conf.intr_type = GPIO_INTR_ANYEDGE;
//    io_conf.mode = GPIO_MODE_INPUT;
//
//    io_conf.pull_down_en = false;
//    io_conf.pull_up_en = true;
//    gpio_config( &io_conf );
}

static bool n2k_send_battery_status( int index, can_message_t *message ) {
    static uint8_t sid = 0;

    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }
    if ( index == 0 ) {
        sid++;
    }
    message->pgn = N2K_PGN_BATTERY_STATUS;
    message->dst = 0;
    message->prio = 0;
    message->src = 0;
    message->len = sizeof( pgn_battery_status_t );
    pgn_battery_status_t *data = (pgn_battery_status_t *) message->data;

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
    data->current = 0; // not available
    data->instance = channel_data.instance;
    data->sid = sid;
    data->temperature = 0; // not available
    data->voltage = channel_data.value;
    return true;
}

static void determine_device_type() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT21 | BIT26;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = true;
    gpio_config( &io_conf );

    device_type = ( gpio_get_level( GPIO_NUM_26 ) << 1 ) |
                  gpio_get_level( GPIO_NUM_21 );
    ESP_LOGI( LOG, "device type %d", device_type );
}

void app_main() {
    determine_device_type();

    ESP_ERROR_CHECK( esp_event_loop_create_default());
//    main_main();
//    set_pins();
    nk2_main();

    switch ( device_type ) {
        case DEVICE_TYPE_GAUGE_DISPLAY:
            break;
        case DEVICE_TYPE_BATTERY_MONITOR:
            adc_main();
            nk2_register_sender( "battery", N2K_PGN_BATTERY_STATUS_INTERVAL, n2k_send_battery_status );
            break;
        default:
            // TODO fail
            ESP_LOGE( LOG, "Unhandled device type %c%c",
                      device_type & 2 ? '1' : '0',
                      device_type & 1 ? '1' : '0' );
            break;
    }

//    esp_sleep_enable_timer_wakeup(1000000);
//    esp_sleep_enable_ext0_wakeup();
//    esp_sleep_enable_gpio_wakeup();
//    esp_light_sleep_start();
}
