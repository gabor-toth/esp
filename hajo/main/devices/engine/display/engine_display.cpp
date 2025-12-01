#include "driver/gpio.h"
#include "engine_display.h"
#include "engine_display_internal.h"
#include "engine_display_draw.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"

static const char *TAG = "display";

static int myDeviceIndex;
display_data_t data = { 0, 0, { 0 } };

/*
 * PNG to XBM:
 * - open PNG in Gimp
 * - Image/Mode/Indexed: choose black&white
 * - Image/Resize image: lock aspect ratio, set size to 16
 * - File/Export: change extension to .xbm
 * - Copy file content here
 */

static void process_incoming_pgn( const tN2kMsg &message );

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
    NMEA2000.AttachMsgHandler( new N2kIncomingMessageHandler( &NMEA2000, process_incoming_pgn ) );
}

static void process_incoming_pgn( const tN2kMsg &message ) {

}

void engine_display_main( int iDev ) {
    gpio_config_t gpioConfig;
    gpioConfig.pin_bit_mask = 1 << PIN_BACKLIGHT;
    gpioConfig.mode = GPIO_MODE_OUTPUT;
    gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpioConfig.intr_type = GPIO_INTR_DISABLE;
    gpio_config( &gpioConfig );

    myDeviceIndex = iDev;
    setup_n2k_device( iDev );

    data.rpm = data.hours = 0;
    data.alerts = 0;

    engine_display_setup_display();
    engine_display_draw_screen();
}

void engine_display_test() {
    engine_display_setup_display();
    data.alert_flags.water_temperature = 1;
    data.rpm = data.hours = 0;
    engine_display_draw_screen();
}
