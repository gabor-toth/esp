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

static int myDeviceIndex;

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
    u8g2_esp32_hal.clock_speed_hz = 1000000;
    u8g2_esp32_hal.device_flags = SPI_DEVICE_POSITIVE_CS;
    u8g2_esp32_hal.reset = PIN_RESET;
    u8g2_esp32_hal_init( u8g2_esp32_hal );
}

static void drawNumber( u8g2_t &u8g2, int x, int y, int fontSize, int value ) {
    int d = fontSize/3*2;
    char s [2];
    s[1] = 0;
    for( int i = 3; i >= 0; i--, value /=10 ) {
        s[0] = ( value % 10) + '0';
        u8g2_DrawStr( &u8g2, x+i*d, y, s );
    }
}

static void demo_screen() {
    u8g2_t u8g2;  // a structure which will contain all the data for one display
    u8g2_Setup_st7920_s_128x64_f(
            &u8g2, U8G2_R2, u8g2_esp32_spi_byte_cb,
            u8g2_esp32_gpio_and_delay_cb );  // init u8g2 structure

    u8g2_InitDisplay( &u8g2 );  // send init sequence to the display, display is in sleep mode after this

    int width = u8g2_GetDisplayWidth(&u8g2 );
    int height = u8g2_GetDisplayHeight(&u8g2 );
    ESP_LOGI(TAG,"display is %dx%d", width, height );

    u8g2_SetPowerSave( &u8g2, 0 );  // wake up display
    ESP_LOGI(TAG,"drawing...");
    u8g2_ClearBuffer( &u8g2 );
    //u8g2_SetDrawColor( &u8g2, 1 );
//    u8g2_DrawLine( &u8g2, width/2, 0, width/2, height-1 );

    u8g2_SetFont( &u8g2, u8g2_font_logisoso32_tr );
    drawNumber( u8g2, 0, height/2+1, 32, 6789 );
    u8g2_SetFont( &u8g2, u8g2_font_logisoso16_tr );
    u8g2_DrawStr( &u8g2, width/2+20, height/2-8, "rpm" );

    u8g2_SetFont( &u8g2, u8g2_font_logisoso16_tr );
    drawNumber( u8g2, width/2+11, height-4, 16, 6789 );
    u8g2_SetFont( &u8g2, u8g2_font_logisoso16_tr );
    u8g2_DrawStr( &u8g2, width/2+16/3*2*5+4, height-4, "h" );

    int iconSize = 16;
    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_embedded_2x_t );
    u8g2_DrawStr( &u8g2, 1, height-3, "\x40" );
    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_embedded_2x_t );
    u8g2_DrawStr( &u8g2, 21, height-3, "\x4f" );

    u8g2_DrawBox( &u8g2, 40, height-3-iconSize-1, iconSize+2, iconSize+2 );
    u8g2_SetDrawColor(&u8g2, 0 );
    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_thing_2x_t );
    u8g2_DrawStr( &u8g2, 41, height-3, "\x4e" );
//    u8g2_SetFont( &u8g2, u8g2_font_open_iconic_embedded_2x_t );
//    u8g2_DrawStr( &u8g2, 48, height-1, "\x40" );

    ESP_LOGI(TAG,"sending...");
    u8g2_SendBuffer( &u8g2 );
    ESP_LOGI(TAG,"done");
}

void engine_display_main( int iDev ) {
    myDeviceIndex = iDev;
    setup_n2k_device( iDev );
    setup_display();
    demo_screen();
}

void engine_display_test() {
    setup_display();
    demo_screen();
}