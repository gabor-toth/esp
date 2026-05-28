#include "adc.h"
#include "cmath"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hajo_rudder.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"
#include "n2k/N2kVarilog.h"

#define PIN_RUDDER_ADC_DRIVE    GPIO_NUM_3
#define ADC_CHANNEL_IN          4            // GPIO_NUM_5
#define USE_CONTINUOUS          0

static const char *LOG = "rudder";

static int r_bottom = 681;
static int r_sensor = 4810;
static int r_top = 16900;
static double u_total = 3.23;
static double i_total = u_total / ( r_bottom + r_sensor + r_top );
static double u_center = i_total * ( r_bottom + r_sensor / 2.0 );
static double u_diff = i_total * ( r_sensor / 2.0 );
static int direction = 1;
static int display_multiplier = 10;
static int max_degree = 90 * display_multiplier;
static double correction_offset = -3.4;
static int myDeviceIndex;

// yn = w × xn + (1 – w) × yn – 1

static void convert_value( int channel, void *user_data, int millivolts, int *display_value, int *correction ) {
    double u = millivolts / 1000.0;
    double value = ( u - u_center ) / u_diff * max_degree * direction + correction_offset * display_multiplier;
    if ( correction != nullptr ) {
        *correction = (int) ( correction_offset * display_multiplier );
    }
    *display_value = (int) lround( value );
    //    if ( *display_value  > max_degree|| *display_value  < -max_degree ) {
    //        *display_value  = INT_MIN;
    //    }
}

static void setup_adc_drive_pins() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( PIN_RUDDER_ADC_DRIVE );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );

    gpio_set_level( PIN_RUDDER_ADC_DRIVE, 1 );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[ ] = {
        N2K_PGN_RUDDER,
        0
    };

    static const unsigned long ReceiveMessages[ ] = {
        0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
        2100, // N2kVersion
        102, // Manufacturer's product code
        "Rudder sensor", // Manufacturer's Model ID
        "1.0.0 (2024-09-24)", // Manufacturer's Software version code
        "1.0.0 (2024-09-24)", // Manufacturer's Model version
        "00000001", // Manufacturer's Model serial code
        0, // CertificationLevel
        1 // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    NMEA2000.SetDeviceInformation( n2k_get_device_id(), // Unique number. Use e.g. Serial number.
        155, // Device function=Rudder
        40, // Device class=Steering and Control Surfaces
        N2K_MANUFACTURER_CODE_VARILOG,
        4, // Marine
        iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_rudder( int index, tN2kMsg &message, int &deviceIndex ) {
    deviceIndex = myDeviceIndex;
    if ( index > 0 ) {
        return false;
    }

    adc_channel_value_t channel_data;
    adc_get_channel_value( 0, &channel_data );

    SetN2kRudder( message,
        channel_data.display_value != INT_MIN ? DegToRad( channel_data.display_value ) : N2kDoubleNA, // radians
        0, // instance
        N2kRDO_NoDirectionOrder,
        N2kDoubleNA // angleOrder
    );
    return true;
}

#if USE_CONTINUOUS
static void adc_callback( int average_raw_value, int average_voltage_value ) {
    tN2kMsg message;
    int display_value = 0;
    convert_value( average_voltage_value, &display_value, nullptr );

    ESP_LOGI( LOG, "Raw: %4d Voltage: %4dmV Display: %5d", average_raw_value, average_voltage_value, display_value );
    SetN2kRudder( message,
        display_value != INT_MIN ? DegToRad( display_value ) : N2kDoubleNA, // radians
        0, // instance
        N2kRDO_NoDirectionOrder,
        N2kDoubleNA // angleOrder
    );
    NMEA2000.SendMsg( message, myDeviceIndex );
}
#endif

static void setup_adc() {
    ESP_LOGI( LOG, "Calculating with Rbottom=%d Rsensor=%d Rtop=%d "
        "Utotal=%.2lfV Itotal=%03duA "
        "Ulow=%03dmV Ucenter=%03dmV Uhigh=%03dmV "
        "Udiff=%03dmV max=%.1lf° correction=%.2f° direction=%d",
        r_bottom,
        r_sensor,
        r_top,
        u_total,
        (int)(i_total*1000000),
        (int)(r_bottom * i_total*1000),
        (int)(u_center*1000),
        (int)((r_bottom+r_sensor) * i_total*1000),
        (int)(u_diff*1000),
        max_degree / (double)display_multiplier,
        correction_offset,
        direction
    );

    adc_add_channel( ADC_CHANNEL_IN, "position", nullptr, 0, convert_value );
#if USE_CONTINUOUS
    adc_main_continuous( adc_callback );
#else
    adc_main_oneshot( false );
#endif
}

void hajo_rudder_main( int iDev ) {
    myDeviceIndex = iDev;
    setup_adc_drive_pins();
    setup_adc();
    setup_n2k_device( iDev );

#if !USE_CONTINUOUS
    nk2_register_sender( send_rudder, "rudder", N2K_PGN_RUDDER_INTERVAL_MS, 65, true );
#endif
}
