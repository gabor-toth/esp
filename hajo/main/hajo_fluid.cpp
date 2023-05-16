#include "cmath"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lib/adc.h"
#include "n2k_parser.h"
#include "n2k_sender.h"

static double fluid_u = 3.20;
static double fluid_rtop = 806;
static double fluid_rbottom = 51.1;
static double fluid_rmes_min = 2;
static double fluid_rmes_max = 180;

typedef struct {
    uint8_t instance;
    uint8_t type;
    gpio_num_t drive_pin;
    uint32_t capacity;
} fluid_user_data;

//static void convert_fluid_level( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
static void convert_fluid_level( uint32_t raw_value, uint32_t *display_value, double *rmes_back ) {
    // Rmes=Rtop/(U/Umes-1)-Rbottom
    // 0% = 2 Ohm, 100% = 180 Ohm
    double rmes = fluid_rtop / ( fluid_u * 1000 / raw_value - 1 ) - fluid_rbottom;
    uint32_t value;
    if ( rmes <= fluid_rmes_min ) {
        value = 0;
    } else if ( rmes >= fluid_rmes_max ) {
        value = 100;
    } else {
        value = lround(( rmes - fluid_rmes_min ) / ( fluid_rmes_max - fluid_rmes_min ) * 100 );
    }
    *display_value = value;
//    if ( correction != nullptr ) {
//        *correction = 0;
    if ( rmes_back != nullptr ) {
        *rmes_back = rmes;
    }
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
            .type = N2kft_Fuel,
            .drive_pin = GPIO_NUM_1,
            .capacity = 60
    };
    adc_add_channel( 1, "uzemanyag_l", &data, sizeof( data ), nullptr );
    adc_add_channel( 6, "uzemanyag_h", &data, sizeof( data ), nullptr );
    gpio_set_level( data.drive_pin, 1 );

    data = {
            .instance = 0,
            .type = N2kft_Water,
            .drive_pin = GPIO_NUM_3,
            .capacity = 85
    };
    adc_add_channel( 3, "viz_bal_l", &data, sizeof( data ), nullptr );
    adc_add_channel( 7, "viz_bal_h", &data, sizeof( data ), nullptr );

    data = {
            .instance = 1,
            .type = N2kft_Water,
            .drive_pin = GPIO_NUM_5,
            .capacity = 85
    };
    adc_add_channel( 5, "viz_jobb_l", &data, sizeof( data ), nullptr );
    adc_add_channel( 8, "viz_jobb_h", &data, sizeof( data ), nullptr );

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
//    int channel_count = adc_number_of_channels();
//    if ( index >= channel_count / 2 ) {
    if ( index >= 1 ) {
        return false;
    }

    adc_channel_value_t channel_data_l;
    adc_channel_value_t channel_data_h;

    fluid_user_data *user_data = static_cast<fluid_user_data *>(adc_get_channel_user_data( index * 2 + 0 ));
    if ( user_data == nullptr ) {
        return true;
    }
//    gpio_set_level( user_data->drive_pin, 1 );
    // TODO add a bit delay to stabilize voltage
    adc_get_channel_value( index * 2 + 0, &channel_data_l );
    adc_get_channel_value( index * 2 + 1, &channel_data_h );
//    gpio_set_level( user_data->drive_pin, 0 );

    uint32_t display_value;
    double rmes;
    convert_fluid_level( channel_data_h.raw_value, &display_value, &rmes );

    ESP_LOGI( "hajo_fluid", "Channel %d %-10s Raw: %4ld Rmes: %lf Display: %5ld",
              channel_data_h.channel,
              channel_data_h.name,
              channel_data_h.raw_value,
              rmes,
              display_value );

    double valueInPercent = display_value / 100.0;
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
