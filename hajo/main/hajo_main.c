#include "config.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "hajo_adc.h"
#include "n2k_png.h"
#include "n2k_protocol.h"
#include "n2k_sender.h"
#include "lib/adc.h"

static const char *LOG = "hajo_main";

// defined by pins 26/21
#define DEVICE_TYPE_GAUGE_DISPLAY 0b111
#define DEVICE_TYPE_BATTERY_MONITOR 0b110
#define DEVICE_TYPE_RESERVED_1 0b01
#define DEVICE_TYPE_RESERVED_0 0b00

static int device_type = 0xff;

static void set_pins() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_DISABLE;
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

static bool n2k_send_fluid_level( int index, can_message_t *message ) {
    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }
    message->pgn = N2K_PGN_FLUID_LEVEL;
    message->dst = 0;
    message->prio = 0;
    message->src = 0;
    message->len = sizeof( pgn_fluid_level_t );
    pgn_fluid_level_t *data = (pgn_fluid_level_t *) message->data;

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
    data->capacity = 0; // not available
    data->instance = channel_data.instance;
    data->level = channel_data.value;
    data->reserved = 0;
    data->type = channel_data.type;
    return true;
}

static void process_incoming_pgn_fluid_level( const can_message_t *message ) {
    pgn_fluid_level_t *data = (pgn_fluid_level_t *) message->data;
    ESP_LOGI( LOG, "packet fluid level %d/%d = %d", data->type, data->instance, data->level );
    if ( data->type == N2K_TANK_TYPE_FUEL ) {
        display_set_value( FUEL, data->instance, data->level );
    } else if ( data->type == N2K_TANK_TYPE_WATER ) {
        display_set_value( WATER, data->instance, data->level );
    }
}

static void process_incoming_pgn_battery_status( const can_message_t *message ) {
    pgn_battery_status_t *data = (pgn_battery_status_t *) message->data;
    ESP_LOGI( LOG, "packet battery status %d = %d", data->instance, data->voltage );
    display_set_value( VOLTAGE, data->instance, data->voltage / 100 );
}

static void process_incoming_pgn( const can_message_t *message ) {
    ESP_LOGI( LOG, "packet pgn %5lx", message->pgn );
    if ( message->pgn == N2K_PGN_FLUID_LEVEL ) {
        process_incoming_pgn_fluid_level( message );
    } else if ( message->pgn == N2K_PGN_BATTERY_STATUS ) {
        process_incoming_pgn_battery_status( message );
    }
}

static void determine_device_type() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
            ( 1 << GPIO_NUM_DEVICE_TYPE_0 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_1 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_2 );
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = true;
    gpio_config( &io_conf );
    vTaskDelay(pdMS_TO_TICKS( 10 ));

    device_type = ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_2 ) << 2 ) |
                  ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_1 ) << 1 ) |
                  gpio_get_level( GPIO_NUM_DEVICE_TYPE_0 );
    ESP_LOGI( LOG, "device type %d", device_type );

    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config( &io_conf );
}

void app_main() {
    determine_device_type();

//    ESP_ERROR_CHECK( nvs_flash_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default());
    n2k_main();

    switch ( device_type ) {
        case DEVICE_TYPE_GAUGE_DISPLAY:
            hajo_adc_main( false );
            nk2_register_sender( "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL, n2k_send_fluid_level );
            display_main();
            n2k_register_receiver( process_incoming_pgn );
            n2k_register_sender_loopback( process_incoming_pgn );
            break;
        case DEVICE_TYPE_BATTERY_MONITOR:
            hajo_adc_main( true );
            nk2_register_sender( "battery", N2K_PGN_BATTERY_STATUS_INTERVAL, n2k_send_battery_status );
            break;
        default:
            // TODO fail
            ESP_LOGE( LOG, "Unhandled device type %c%c%c",
                      device_type & 4 ? '1' : '0',
                      device_type & 2 ? '1' : '0',
                      device_type & 1 ? '1' : '0' );
            break;
    }

//    esp_sleep_enable_timer_wakeup(1000000);
//    esp_sleep_enable_ext0_wakeup();
//    esp_sleep_enable_gpio_wakeup();
//    esp_light_sleep_start();
}
