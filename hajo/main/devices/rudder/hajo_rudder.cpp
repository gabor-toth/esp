#include "adc.h"
#include "cmath"
#include "driver/gpio.h"
#include "hajo_rudder.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"

#define PIN_RUDDER_ADC_DRIVE GPIO_NUM_11

static int battery_rmes = 16900;
static int battery_rtop = 316000;
static int battery_offset = -50;
static double battery_multiplier;

typedef struct {
    uint8_t instance;
    uint32_t capacity_ah;
    uint32_t ripple_voltage_mv;
} battery_user_data;

static void convert_value( uint32_t raw_value, uint32_t *display_value, uint32_t *correction ) {
    double value;
    if ( raw_value <= 100 ) {
        value = 0.0;
        *correction = 0;
    } else {
        // U=(Rtop+Rmes)/Rmes*Umes
        value = raw_value * battery_multiplier + battery_offset;
        *correction = battery_offset;
    }
    if ( value < 0.0 ) {
        value = 0.0;
    }
    *display_value = lround( value );
}

static void setup_adc_drive_pins() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( PIN_RUDDER_ADC_DRIVE );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

static void setup_adc() {
    adc_add_channel( 8, "position", nullptr, 0, convert_value );

    gpio_set_level( PIN_RUDDER_ADC_DRIVE, 1 );
    adc_main( false );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_RUDDER,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            101,                        // Manufacturer's product code
            "Rudder sensor",           // Manufacturer's Model ID
            "1.0.0 (2024-09-24)",        // Manufacturer's Software version code
            "1.0.0 (2024-09-24)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   155,    // Device function=Rudder
                                   40,        // Device class=Steering and Control Surfaces
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_rudder( int index, tN2kMsg &message ) {
    if ( index > 0 ) {
        return false;
    }

    /*
    Wire colours	Expected resistance measurement
    Red to Green	5k Ohms (5000 ohms, +/- 10%) steady
    Blue to Red	    Approx. 1.6k to 3.3k ohms, roughly 2.5k ohms when the wheel is centred (+/-10%)
    Blue to Green	Approx. 3.3k to 1.6k ohms, roughly 2.5k ohms when the wheel is centred (+/-10%)
     */
    adc_channel_value_t channel_data;

    gpio_set_level( PIN_RUDDER_ADC_DRIVE, 1 );
    adc_get_channel_value( 0, &channel_data );
    gpio_set_level( PIN_RUDDER_ADC_DRIVE, 0 );
    /*
    SetN2kRudder( message,
                  channel_data.display_value / 1000.0, // radians
                  0, // instance
                  N2kRDO_NoDirectionOrder,
                  N2kDoubleNA // angleOrder
    );
     */
    return true;
}

void hajo_rudder_main( int iDev ) {
    setup_adc_drive_pins();
    setup_adc();
    setup_n2k_device( iDev );
    nk2_register_sender( send_rudder, "rudder", N2K_PGN_RUDDER_INTERVAL_MS, 65, true );
}
