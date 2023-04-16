#include "lib/nmea2000/n2k_png.h"
#include "lib/adc.h"
#include "n2k_sender.h"
#include "hajo_battery.h"

static int battery_rmes = 16900;
static int battery_rtop = 316000;
static int battery_offset = 0;
static double battery_multiplier;

static void convert_battery_voltage( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
    // U=(Rtop+Rmes)/Rmes*Umes
    *correction = battery_offset;
    double value = raw_value * battery_multiplier;
    if ( raw_value <= 20 ) {
        value = 0.0;
        *correction = 0;
    } else {
        value += battery_offset;
    }
    if ( value < 0.0 ) {
        value = 0.0;
    }
    // PGN 127508 - Battery Status: voltage 0.01V
    *display_value = (uint32_t) value * 100;
}

static void setup_adc() {
    battery_multiplier = ( battery_rmes + battery_rtop ) / (double) battery_rmes;
    adc_add_channel( 0, "motor", 0, 0, convert_battery_voltage );
    adc_add_channel( 1, "munka1", 1, 0, convert_battery_voltage );
    adc_add_channel( 2, "munka2", 2, 0, convert_battery_voltage );
    adc_main( false );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_BATTERY_STATUS,
            N2K_PGN_DC_DETAILED_STATUS,
            N2K_PGN_BATTERY_CONFIGURATION,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            100,                        // Manufacturer's product code
            "Battery monitor",           // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   170,    // Device function=Battery.
                                   35,        // Device class=Electrical Generation.
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_battery_status( int index, tN2kMsg &message ) {
    static uint8_t sid = 0;

    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }
    if ( index == 0 ) {
        sid++;
    }

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
    SetN2kDCBatStatus( message,
                       channel_data.instance,
                       channel_data.value,
                       N2kDoubleNA, // current
                       N2kDoubleNA, // temperature
                       sid
    );
    return true;
}

static bool send_dc_status( int index, tN2kMsg &message ) {
    static uint8_t sid = 0;

    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }
    if ( index == 0 ) {
        sid++;
    }

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
//    SetN2kDCStatus( N2kMsg, 1, 1, N2kDCt_Battery, 56, 92, 38500, 0.012 );
    SetN2kDCStatus( message,
                    sid,
                    channel_data.instance,
                    N2kDCt_Battery,
                    N2kUInt8NA, //StateOfCharge
                    N2kUInt8NA, // StateOfHealth,
                    N2kDoubleNA, // TimeRemaining,
                    N2kDoubleNA, // RippleVoltage
                    N2kDoubleNA // Capacity
    );
    return true;
}

static bool send_battery_config( int index, tN2kMsg &message ) {
    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }

    adc_channel_value_t channel_data;
    adc_get_channel_value( index, &channel_data );
//    SetN2kBatConf( N2kMsg, 1, N2kDCbt_Gel, N2kDCES_Yes, N2kDCbnv_12v, N2kDCbc_LeadAcid, AhToCoulomb( 420 ), 53, 1.251, 75 );
    SetN2kBatConf( message,
                   channel_data.instance,
                   N2kDCbt_Gel,
                   N2kDCES_No,
                   N2kDCbnv_12v,
                   N2kDCbc_LeadAcid,
                   AhToCoulomb( 420 ),
                   N2kInt8NA,
                   N2kDoubleNA,
                   N2kInt8NA
    );
    return true;
}

void hajo_battery_main( int iDev ) {
    setup_adc();
    setup_n2k_device( iDev );
    nk2_register_sender( send_battery_status, "battery", N2K_PGN_BATTERY_STATUS_INTERVAL_MS, 60, true );
    nk2_register_sender( send_dc_status, "battery", N2K_PGN_DC_DETAILED_STATUS_INTERVAL_MS, 70, true );
    nk2_register_sender( send_battery_config, "battery", N2K_PGN_BATTERY_CONFIGURATION_INTERVAL_MS, 80, true );
}
