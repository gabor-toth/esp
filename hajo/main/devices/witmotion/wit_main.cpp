#include "esp_log.h"
#include "wit_main.h"
#include "wit_sensor.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_ATTITUDE,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            100,                        // Manufacturer's product code
            "WitMotion",                 // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=heading/attitude/pitch
                                   60,        // Device class=Navigation
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool n2k_send_attitude( int index, tN2kMsg &message ) {
    static uint8_t sid = 0;

    if ( index == 0 ) {
        sid++;
    }

    switch ( index ) {
        case 0:
            if ( wit_sensor_is_available() ) {
                float pitch, roll;
                wit_sensor_get_pitch_and_roll( &pitch, &roll );
                SetN2kAttitude( message, sid, 0.0, DegToRad( pitch ), DegToRad( roll ) );
            }
            return true;
        case 1:
            if ( wit_sensor_is_available() ) {
                float heading;
                wit_sensor_get_heading( &heading );
                SetN2kTrueHeading( message, sid, DegToRad( heading ) );
            }
            return true;
        default:
            return false;
    }
}

void wit_main( int iDev ) {
    wit_sensor_start();
    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_attitude, "attitude", N2K_PGN_ATTITUDE_INTERVAL_MS, 320, true );
}
