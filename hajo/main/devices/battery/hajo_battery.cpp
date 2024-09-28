#include "cmath"
#include "n2k/n2k_struct_parser.h"
#include "adc.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"
#include "hajo_battery.h"

static int battery_rmes = 16900;
static int battery_rtop = 316000;
static int battery_offset = -50;
static double battery_multiplier;

typedef struct {
    uint8_t instance;
    int capacity_ah;
    int ripple_voltage_mv;
} battery_user_data;

static void convert_battery_voltage( int millivolts, int *display_value, int *correction ) {
    double value;
    if ( millivolts <= 100 ) {
        value = 0.0;
        *correction = 0;
    } else {
        // U=(Rtop+Rmes)/Rmes*Umes
        value = millivolts * battery_multiplier + battery_offset;
        *correction = battery_offset;
    }
    if ( value < 0.0 ) {
        value = 0.0;
    }
    *display_value = lround( value );
}

static void setup_adc() {
    battery_multiplier = ( battery_rmes + battery_rtop ) / (double) battery_rmes;

    battery_user_data data;
    data = {
            .instance = 0,
            .capacity_ah = 900,
            .ripple_voltage_mv = 10000
    };
    adc_add_channel( 2, "motor", &data, sizeof( data ), convert_battery_voltage );

    data = {
            .instance = 1,
            .capacity_ah = 900,
            .ripple_voltage_mv = 10000
    };
    adc_add_channel( 4, "munka1", &data, sizeof( data ), convert_battery_voltage );

    data = {
            .instance = 2,
            .capacity_ah = 1100,
            .ripple_voltage_mv = 10000
    };
    adc_add_channel( 6, "munka2", &data, sizeof( data ), convert_battery_voltage );

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
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
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
    battery_user_data *user_data = static_cast<battery_user_data *>(channel_data.user_data);
    SetN2kDCBatStatus( message,
                       user_data->instance,
                       channel_data.display_value / 1000.0, // mV -> V
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

    adc_channel_data_t channel_data;
    adc_get_channel_data( index, &channel_data );
    battery_user_data *user_data = static_cast<battery_user_data *>(channel_data.user_data);
//    SetN2kDCStatus( N2kMsg, 1, 1, N2kDCt_Battery, 56, 92, 38500, 0.012 );
    SetN2kDCStatus( message,
                    sid,
                    user_data->instance,
                    N2kDCt_Battery,
                    N2kUInt8NA, //StateOfCharge
                    N2kUInt8NA, // StateOfHealth,
                    N2kDoubleNA, // TimeRemaining,
                    N2kDoubleNA, // RippleVoltage
                    N2kDoubleNA // Remaining Capacity
    );
    return true;
}

static bool send_battery_config( int index, tN2kMsg &message ) {
    int channel_count = adc_number_of_channels();
    if ( index >= channel_count ) {
        return false;
    }

    adc_channel_data_t channel_data;
    adc_get_channel_data( index, &channel_data );
    battery_user_data *user_data = static_cast<battery_user_data *>(channel_data.user_data);
//    SetN2kBatConf( N2kMsg, 1, N2kDCbt_Gel, N2kDCES_Yes, N2kDCbnv_12v, N2kDCbc_LeadAcid, AhToCoulomb( 420 ), 53, 1.251, 75 );
    SetN2kBatConf( message,
                   user_data->instance,
                   N2kDCbt_Gel,
                   N2kDCES_No,
                   N2kDCbnv_12v,
                   N2kDCbc_LeadAcid,
                   user_data->capacity_ah != 0 ? AhToCoulomb( user_data->capacity_ah ) : N2kDoubleNA,
                   N2kInt8NA,
                   N2kDoubleNA,
                   N2kInt8NA
    );
    return true;
}

void hajo_battery_main( int iDev ) {
    setup_adc();
    setup_n2k_device( iDev );
    nk2_register_sender( send_battery_status, "battery_status", N2K_PGN_BATTERY_STATUS_INTERVAL_MS, 60, true );
    nk2_register_sender( send_dc_status, "dc_status", N2K_PGN_DC_DETAILED_STATUS_INTERVAL_MS, 70, true );
    nk2_register_sender( send_battery_config, "battery_config", N2K_PGN_BATTERY_CONFIGURATION_INTERVAL_MS, 80, true );
}
