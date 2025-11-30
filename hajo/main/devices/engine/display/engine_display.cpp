#include "esp_log.h"
#include "esp_timer.h"
#include "engine_display.h"
#include "n2k/n2k_sender.h"

extern "C" {
#include "u8g2_esp32_hal.h"
}

static const char *TAG = "display";

#define PIN_CLK     GPIO_NUM_7      // E
#define PIN_MOSI    GPIO_NUM_5      // RW
#define PIN_CS      GPIO_NUM_3      // RS
#define PIN_RESET   GPIO_NUM_9      // RST

// GND
// VCC = 5V
// VO = 10k pot middle pin
// BLA = LED+ Backlight 5V
// BLK = LED– Backlight GND
// PSB = LOW -> SPI mode

typedef struct {
    int16_t rpm;
    int16_t hours;
    union {
        uint8_t alerts;
        struct {
            unsigned charger : 1;
            unsigned oil_pressure : 1;
            unsigned water_temperature : 1;
        } alert_flags;
    };
} display_data_t;

static int myDeviceIndex;
static u8g2_t u8g2;  // a structure which will contain all the data for one display
static display_data_t data = { 0, 0, { 0 } };

#define ICON_SIZE 16

/*
 * PNG to XBM:
 * - open PNG in Gimp
 * - Image/Mode/Indexed: choose black&white
 * - Image/Resize image: lock aspect ratio, set size to 16
 * - File/Export: change extension to .xbm
 * - Copy file content here
 */


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


static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            0
    };

    static const unsigned long ReceiveMessages[] = {
            N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
            N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                    // N2kVersion
            107,                     // Manufacturer's product code
            "Engine display",      // Manufacturer's Model ID
            "1.0.0 (2025-11-22)",    // Manufacturer's Software version code
            "1.0.0 (2025-11-22)",    // Manufacturer's Model version
            "00000001",              // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                        // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   130,    // Display
                                   120,        // Device class=Display
                                   2046,  // Just chosen free from code list on https://github.com/ieb/EngineMonitor/blob/master/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                   4,       // Marine
                                   iDev
    );
    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static void setup_display() {
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
}

static void draw_number( u8g2_t &u8g2, int x, int y, int fontSize, int value ) {
    int d = fontSize/3*2;
    char s [2];
    s[1] = 0;
    for( int i = 3; i >= 0; i--, value /=10 ) {
        s[0] = ( value % 10) + '0';
        u8g2_DrawStr( &u8g2, x+i*d, y, s );
    }
}

static void draw_icon( int x, int y, const char* character, bool alert_state ) {
    u8g2_SetDrawColor( &u8g2, 1 );
    if ( alert_state ) {
        u8g2_DrawBox( &u8g2, x - 1, y - ICON_SIZE - 1, ICON_SIZE + 2, ICON_SIZE + 2 );
        u8g2_SetDrawColor( &u8g2, 0 );
    }
    u8g2_DrawStr( &u8g2, x, y, character );
}

static void draw_icon_xbm( int x, int y, const uint8_t *bitmap, bool alert_state ) {
    u8g2_SetDrawColor( &u8g2, 1 );
    if ( alert_state ) {
        u8g2_DrawBox( &u8g2, x - 1, y - ICON_SIZE - 1, ICON_SIZE + 2, ICON_SIZE + 2 );
        u8g2_SetDrawColor( &u8g2, 0 );
    }
    u8g2_DrawXBM( &u8g2, x, y-ICON_SIZE, ICON_SIZE, ICON_SIZE, bitmap );
}

static void draw_screen() {
    int width = u8g2_GetDisplayWidth(&u8g2 );
    int height = u8g2_GetDisplayHeight(&u8g2 );

    u8g2_SetPowerSave( &u8g2, 0 );  // wake up display
    u8g2_ClearBuffer( &u8g2 );

    u8g2_SetFont( &u8g2, u8g2_font_logisoso32_tr );
    draw_number( u8g2, 0, height / 2 + 1, 32, data.rpm );
    u8g2_SetFont( &u8g2, u8g2_font_logisoso16_tr );
    u8g2_DrawStr( &u8g2, width/2+20, height/2-8, "rpm" );

    draw_number( u8g2, width / 2 + 11, height - 4, 16, data.hours );
    u8g2_DrawStr( &u8g2, width/2+16/3*2*5+4, height-4, "h" );

    u8g2_SetDrawColor( &u8g2, 1 );
    draw_icon_xbm( 1, height-3, car_oil_bits, data.alert_flags.charger );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_thing_2x_t );
//    draw_icon( 1, height-3, "\x40" , data.alert_flags.charger);

    draw_icon_xbm( 21, height-3, car_battery_bits, data.alert_flags.oil_pressure );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_embedded_2x_t );
//    draw_icon( 21, height-3, "\x4f" , data.alert_flags.water_temperature);

    draw_icon_xbm( 41, height-3, thermometer_bits, data.alert_flags.water_temperature );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_thing_2x_t );
//    draw_icon( 41, height-3, "\x4e" , data.alert_flags.oil_pressure);

    u8g2_SendBuffer( &u8g2 );
}

void engine_display_main( int iDev ) {
    myDeviceIndex = iDev;
    setup_n2k_device( iDev );
    setup_display();
    draw_screen();
}

void engine_display_test() {
    setup_display();
    data.alert_flags.water_temperature = 1;
    data.rpm = data.hours = 6789;
    draw_screen();
}