#include "adc.h"
#include "cmath"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hajo_rudder.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"

#define PIN_RUDDER_ADC_DRIVE GPIO_NUM_11
#define USE_CONTINUOUS 1

static const char *LOG = "rudder";

static int r_bottom = 680;
static int r_sensor = 5400;
static int r_top = 16900;
static double u_total = 3.3;
static double i_total = u_total / ( r_bottom + r_sensor + r_top );
static double u_center = i_total * ( r_bottom + r_sensor / 2.0 );
static double u_diff = i_total * ( r_sensor / 2.0 );
static int direction = -1;
static int display_multiplier = 10;
static int max_degree = 90 * display_multiplier;
static int correction_offset = 2;
static int display_scale = 1;

static void convert_value( int millivolts, int *display_value, int *correction ) {
    double u = millivolts / 1000.0;
    double value = ( u - u_center ) / ( u_diff / 2 ) * max_degree * direction + correction_offset;
    if ( correction != nullptr) {
        *correction = correction_offset;
    }
    *display_value = lround( value * display_scale );
    if ( *display_value  > max_degree*display_scale || *display_value  < -max_degree*display_scale ) {
        //*display_value  = INT_MIN;
        if ( correction != nullptr) {
            *correction = 0;
        }
    }
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

    adc_channel_value_t channel_data;
    adc_get_channel_value( 0, &channel_data );

    SetN2kRudder( message,
                  channel_data.display_value != INT_MIN ? DegToRad(channel_data.display_value/(double)display_scale) :  N2kDoubleNA , // radians
                  0, // instance
                  N2kRDO_NoDirectionOrder,
                  N2kDoubleNA // angleOrder
    );
    return true;
}

static void adc_callback(  int average_raw_value, int average_voltage_value ) {
    tN2kMsg message;
    int display_value = 0;
    convert_value(average_voltage_value, &display_value, nullptr);

    ESP_LOGI( LOG, "Raw: %4d Voltage: %4dmV Display: %5d", average_raw_value, average_voltage_value, display_value );
    SetN2kRudder( message,
                  display_value != INT_MIN ? DegToRad(display_value/(double)display_scale) :  N2kDoubleNA , // radians
                  0, // instance
                  N2kRDO_NoDirectionOrder,
                  N2kDoubleNA // angleOrder
    );
    NMEA2000.SendMsg( message );
}

static void setup_adc() {
    ESP_LOGI(LOG,"Calculating with Rbottom=%d Rsensor=%d Rtop=%d Utotal=%.2lfV Itotal=%03duA Ucenter=%03dmV Udiff=%03dmV max=%.1lf° correction=%d° direction=%d",
             r_bottom,
             r_sensor,
             r_top,
             u_total,
             (int)(i_total*1000000),
             (int)(u_center*1000),
             (int)(u_diff*1000),
             max_degree / (double)display_multiplier,
             correction_offset,
             direction
    );

    adc_add_channel( 8, "position", nullptr, 0, convert_value );
#if USE_CONTINUOUS
    adc_main_continuous(adc_callback);
#else
    adc_main_oneshot( false );
#endif
}

void hajo_rudder_main( int iDev ) {
    setup_adc_drive_pins();
    setup_adc();
    setup_n2k_device( iDev );

    ESP_LOGI(LOG, "sizeof int=%d long=%d uint32_t=%d float=%d double=%d", sizeof(int), sizeof(long), sizeof(uint32_t), sizeof(float), sizeof(double));
#if !USE_CONTINUOUS
    nk2_register_sender( send_rudder, "rudder", N2K_PGN_RUDDER_INTERVAL_MS*10, 65, true );
#endif
}
