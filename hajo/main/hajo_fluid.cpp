#include "driver/gpio.h"
#include "lib/adc.h"
#include "lib/nmea2000/n2k_png.h"
#include "n2k_sender.h"

static double fluid_u = 3.32;
static double fluid_rtop = 680;
//static double fluid_rtop = 634;
static double fluid_rmes_min = 2;
static double fluid_rmes_max = 180;

typedef struct {
    uint8_t instance;
    uint8_t type;
    gpio_num_t drive_pin;
    uint32_t capacity;
} fluid_user_data;

static void convert_fluid_level( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
    // Rmes=Rtop/(U/Umes-1)
    // 0% = 2 Ohm, 100% = 180 Ohm
    double rmes = fluid_rtop / ( fluid_u * 1000 / raw_value - 1 );
    uint32_t value;
    if ( rmes <= fluid_rmes_min ) {
        value = 0;
    } else if ( rmes >= fluid_rmes_max ) {
        value = 100;
    } else {
        value = (uint32_t) (( rmes - fluid_rmes_min ) / ( fluid_rmes_max - fluid_rmes_min ) * 100 );
    }
    *display_value = value;
    *correction = 0;
}

static void setup_drive_pins() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( GPIO_NUM_1 ) | BIT( GPIO_NUM_3 ) | BIT( GPIO_NUM_5 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

static void setup_adc() {
    fluid_user_data data;


    data = {
            .instance = 0,
            .type = N2K_TANK_TYPE_FUEL,
            .drive_pin = GPIO_NUM_1,
            .capacity = 60
    };
    adc_add_channel( 1, "uzemanyag l", &data, sizeof( data ), convert_fluid_level );
    adc_add_channel( 6, "uzemanyag h", &data, sizeof( data ), convert_fluid_level );

    data = {
            .instance = 0,
            .type = N2K_TANK_TYPE_WATER,
            .drive_pin = GPIO_NUM_3,
            .capacity = 85
    };
    adc_add_channel( 3, "viz bal l", &data, sizeof( data ), convert_fluid_level );
    adc_add_channel( 7, "viz bal h", &data, sizeof( data ), convert_fluid_level );

    data = {
            .instance = 1,
            .type = N2K_TANK_TYPE_WATER,
            .drive_pin = GPIO_NUM_5,
            .capacity = 85
    };
    adc_add_channel( 5, "viz jobb l", &data, sizeof( data ), convert_fluid_level );
    adc_add_channel( 8, "viz jobb h", &data, sizeof( data ), convert_fluid_level );

    adc_main( false );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_FLUID_LEVEL,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                       // N2kVersion
            101,                       // Manufacturer's product code
            "Fluid level",              // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",       // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",   // Manufacturer's Model version
            "00000001",           // Manufacturer's Model serial code
            0,                      // CertificationLevel
            1                        // LoadEquivalency
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
    fluid_user_data *user_data = static_cast<fluid_user_data *>(channel_data.user_data);
//    gpio_set_level( StandbyPin, 0 );

    double valueInPercent = channel_data.value / 100.0;
    SetN2kFluidLevel( message,
                      user_data->instance,
                      (tN2kFluidType) user_data->type,
                      valueInPercent,
                      user_data->capacity != 0 ? user_data->capacity : N2kDoubleNA // capacity
    );
    return true;
}

void hajo_fluid_main( int iDev ) {
    setup_drive_pins();
    setup_adc();
    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_fluid_level, "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL_MS, 250, true );
}
