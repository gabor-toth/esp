#include "esp_log.h"
#include "esp_timer.h"
#include "engine_sender.h"
#include "n2k/n2k_sender.h"

//static const char *TAG = "sender";

static int myDeviceIndex;

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
            N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                    // N2kVersion
            103,                     // Manufacturer's product code
            "Engine sender",      // Manufacturer's Model ID
            "1.0.0 (2025-11-22)",    // Manufacturer's Software version code
            "1.0.0 (2025-11-22)",    // Manufacturer's Model version
            "00000001",              // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                        // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   160,    // Engine Gateway
                                   50,        // Device class=Propulsion
                                   2046,  // Just chosen free from code list on https://github.com/ieb/EngineMonitor/blob/master/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_temperature( int index, tN2kMsg &message, int &deviceIndex ) {
    deviceIndex = myDeviceIndex;
    if ( index > 0 ) {
        return false;
    }
    return true;
}

void engine_sender_main( int iDev ) {
    myDeviceIndex = iDev;
    setup_n2k_device( iDev );

    nk2_register_sender( send_temperature, "engine_sender", N2K_PGN_TEMPERATURE_INTERVAL_MS * 10, 65, true );
}
