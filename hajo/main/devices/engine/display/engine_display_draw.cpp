#include "engine_display_internal.h"
#include "engine_display_draw.h"
#include "esp_log.h"
#include "esp_timer.h"

extern "C" {
#include "u8g2_esp32_hal.h"
}

static const char *TAG = "display";
static bool show_leading_zeroes = false;

#define ICON_SIZE 16

#define car_battery_width 16
#define car_battery_height 16
static unsigned char car_battery_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0xff, 0xff, 0xff, 0xff,
        0xfc, 0x3f, 0xfe, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x3e, 0x7c, 0x7e, 0x7f,
        0xfe, 0x7f, 0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00 };

#define car_oil_width 16
#define car_oil_height 16
static unsigned char car_oil_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0xe0, 0x00, 0xff, 0xe7,
        0xfd, 0x7f, 0xfd, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xfc, 0xcf, 0xf8, 0xe7,
        0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define thermometer_width 16
#define thermometer_height 16
static unsigned char thermometer_bits[] = {
        0xc0, 0x03, 0x40, 0x02, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06,
        0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xb0, 0x0d, 0xd0, 0x0b,
        0xd0, 0x0b, 0xb0, 0x0d, 0x60, 0x06, 0xc0, 0x03 };

static u8g2_t u8g2;  // a structure which will contain all the data for one display
static QueueHandle_t display_event_queue;

static void draw_number( int x, int y, int fontSize, int value ) {
    // draw each digit individually to minimize space between them
    int d = fontSize/3*2;
    char s [2];
    s[1] = 0;
    for( int i = 3; i >= 0; i--  ) {
        if ( value == 0 && !show_leading_zeroes && i != 3 ) {
            break;
        }
        if ( value >= 0 ) {
            s[ 0 ] = ( value % 10 ) + '0';
            value /=10;
        } else {
            s[0] = '-';
        }
        u8g2_DrawStr( &u8g2, x+i*d, y, s );
    }
}

#if 0
static void draw_icon( int x, int y, const char* character, bool alert_state ) {
    u8g2_SetDrawColor( &u8g2, 1 );
    if ( alert_state ) {
        u8g2_DrawBox( &u8g2, x - 1, y - ICON_SIZE - 1, ICON_SIZE + 2, ICON_SIZE + 2 );
        u8g2_SetDrawColor( &u8g2, 0 );
    }
    u8g2_DrawStr( &u8g2, x, y, character );
}
#endif

static void draw_icon_xbm( int x, int y, const uint8_t *bitmap, bool alert_state ) {
    u8g2_SetDrawColor( &u8g2, 1 );
    if ( alert_state && displayData.flashState ) {
        u8g2_DrawBox( &u8g2, x - 1, y - ICON_SIZE - 1, ICON_SIZE + 2, ICON_SIZE + 2 );
        u8g2_SetDrawColor( &u8g2, 0 );
    }
    u8g2_DrawXBM( &u8g2, x, y-ICON_SIZE, ICON_SIZE, ICON_SIZE, bitmap );
}

static void draw_screen() {
    int width = u8g2_GetDisplayWidth(&u8g2 );
    int height = u8g2_GetDisplayHeight(&u8g2 );

    u8g2_SetPowerSave( &u8g2, 0 );  // wake up display
    gpio_set_level(PIN_BACKLIGHT, 1 );
    u8g2_ClearBuffer( &u8g2 );

    u8g2_SetDrawColor( &u8g2, 1 );
    u8g2_SetFont( &u8g2, u8g2_font_logisoso32_tr );
    draw_number( 0, height / 2 + 1, 32, displayData.rpm );
    u8g2_SetFont( &u8g2, u8g2_font_logisoso16_tr );
    u8g2_DrawStr( &u8g2, width/2+20, height/2-6, "rpm" );

    draw_number( width / 2 + 11, height - 4, 16, displayData.hours );
    u8g2_DrawStr( &u8g2, width/2+16/3*2*5+4, height-4, "h" );

    int x = 1;
    draw_icon_xbm( x, height-3, car_oil_bits, displayData. oilPressureFailure );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_thing_2x_t );
//    draw_icon( x, height-3, "\x40" , data.alert_flags.charger);
    x += 20;

    draw_icon_xbm( x, height-3, thermometer_bits, displayData.coolingWaterTemperatureFailure );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_thing_2x_t );
//    draw_icon( x, height-3, "\x4e" , data.alert_flags.oil_pressure);
    x += 20;

    draw_icon_xbm( x, height-3, car_battery_bits, displayData.chargerFailure );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_embedded_2x_t );
//    draw_icon( x, height-3, "\x4f" , data.alert_flags.water_temperature);


    u8g2_SendBuffer( &u8g2 );
}

_Noreturn static void task_display( void *arg ) {
    (void) arg;

    for ( ;; ) {
        int dummy;

        if ( !xQueueReceive( display_event_queue, &dummy, portMAX_DELAY )) {
            continue;
        }
        draw_screen();
    }
}

void engine_display_setup_display() {
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.bus.spi.clk = PIN_CLK;
    u8g2_esp32_hal.bus.spi.cs = PIN_CS;
    u8g2_esp32_hal.bus.spi.mosi = PIN_MOSI;
    u8g2_esp32_hal.reset = PIN_RESET;
    u8g2_esp32_hal_init( u8g2_esp32_hal );

    u8g2_Setup_st7565_ea_dogm128_f(
            &u8g2, U8G2_R2, u8g2_esp32_spi_byte_cb,
            u8g2_esp32_gpio_and_delay_cb );  // init u8g2 structure

    // u8g2_m_16_8_f
    u8g2_Setup_st7920_s_128x64_f(
            &u8g2, U8G2_R2, u8g2_esp32_spi_byte_cb,
            u8g2_esp32_gpio_and_delay_cb );  // init u8g2 structure

    u8g2_InitDisplay( &u8g2 );  // send init sequence to the display, display is in sleep mode after this

    int width = u8g2_GetDisplayWidth(&u8g2 );
    int height = u8g2_GetDisplayHeight(&u8g2 );
    ESP_LOGI(TAG,"display is %dx%d", width, height );

    display_event_queue = xQueueCreate( 10, sizeof( int ));
    xTaskCreate( task_display, TAG, 2048, nullptr, 10, nullptr );
}

void engine_display_draw_screen() {
    int dummy = 0;
    xQueueSendToBack( display_event_queue, &dummy, 0 );
}
