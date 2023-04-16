//#include "display_main.h"
#include "hajo_adc.h"
#include "lib/nmea2000/n2k_png.h"
#include "lib/adc.h"
#include "n2k_sender.h"

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_FLUID_LEVEL,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            101,                        // Manufacturer's product code
            "Liquid level + display",    // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   150,    // Device function=Fluid level
                                   75,        // Device class=Sensor Communication Interface
                                   2046, // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool n2k_send_fluid_level( int index, tN2kMsg &message ) {
    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
    SetN2kFluidLevel( message,
                      channel_data.instance,
                      (tN2kFluidType) channel_data.type,
                      channel_data.value,
                      N2kDoubleNA // capacity
    );
    return true;
}

void hajo_fluid_main( int iDev ) {
    setup_n2k_device( iDev );
    hajo_adc_main( false );
    nk2_register_sender( n2k_send_fluid_level, "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL_MS, 250, true );
}
