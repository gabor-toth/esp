#include "esp_log.h"
#include "wit_main.h"
#include "wit_sensor.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"
#include "n2k/N2kVarilog.h"

static int myDeviceIndex;

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_ATTITUDE,
            N2K_PGN_HEADING,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            121,                        // Manufacturer's product code
            "WitMotion",                 // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=heading/attitude/pitch
                                   60,        // Device class=Navigation
                                   N2K_MANUFACTURER_CODE_VARILOG,
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool n2k_send_attitude( int index, tN2kMsg &message, int &deviceIndex ) {
    static uint8_t sid = 0;

    deviceIndex = myDeviceIndex;
    if ( index == 0 ) {
        sid++;
    }

    if ( index > 0 || !wit_sensor_is_available() ) {
        return false;
    }
    float pitch, roll;
    wit_sensor_get_pitch_and_roll( &pitch, &roll );
    SetN2kAttitude( message, sid, 0.0, DegToRad( pitch ), DegToRad( roll ) );
    return true;
}

static bool n2k_send_heading( int index, tN2kMsg &message, int &deviceIndex ) {
    static uint8_t sid = 0;

    deviceIndex = myDeviceIndex;
    if ( index == 0 ) {
        sid++;
    }

    if ( index > 0 || !wit_sensor_is_available() ) {
        return false;
    }
    float heading;
    wit_sensor_get_heading( &heading );
    SetN2kTrueHeading( message, sid, DegToRad( heading ) );
    return true;
}

void wit_main( int iDev ) {
    myDeviceIndex = iDev;
    wit_sensor_start();
    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_heading, "heading", N2K_PGN_HEADING_INTERVAL_MS, 325, true );
    nk2_register_sender( n2k_send_attitude, "attitude", N2K_PGN_ATTITUDE_INTERVAL_MS, 320, true );
}
