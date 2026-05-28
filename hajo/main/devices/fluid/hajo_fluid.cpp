#include "cmath"
#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"
#include "n2k/N2kVarilog.h"

//static double fluid_rtop = 806;
static double fluid_rbottom = 51.1;
/*
* In Europe, boat fuel level sensors (sending units) typically operate on an electrical resistance range of
* 0 (Empty) to 180 (Full). Some European senders and gauges (particularly legacy VDO equipment) may scale slightly differently,
* from 0 to 190.
 */
static double fluid_rmes_min = 2;
static double fluid_rmes_max = 190;
static int myDeviceIndex;

static const char *TAG = "hajo_fluid";
//#define ESP_LOG ESP_LOGD

#define PIN_WATER_0 GPIO_NUM_12
#define PIN_WATER_1 GPIO_NUM_11
#define PIN_WATER_2 GPIO_NUM_8
#define PIN_WATER_3 GPIO_NUM_9
#define PIN_WATER_4 GPIO_NUM_6
#define PIN_ADC_1_DRIVE GPIO_NUM_3
#define PIN_ADC_1_HIGH  GPIO_NUM_5
#define PIN_ADC_1_LOW   GPIO_NUM_7

#define MAX_FLUID_COUNT 3

typedef struct {
    uint8_t instance;
    uint8_t type;
    gpio_num_t drive_gpio_pin;
    uint32_t capacity;
    uint8_t adc_channel_low;
    uint8_t adc_channel_high;
} fluid_user_data;

int fluid_count = 0;
fluid_user_data fluid_data[ MAX_FLUID_COUNT ];

//static void convert_fluid_level( int voltageBottom, int *display_value, int *correction ) {
static void convert_fluid_level( int voltageBottom, int voltageTop, int *display_value, double *rmes_back ) {
    int value;
    double i, rmes;
    if ( voltageBottom >= 100 ) {
        // Rmes=Rtop/(U/Umes-1)-Rbottom
        // 0% = 0 Ohm, 100% = 190 Ohm
        i = ( voltageBottom / 1000.0 ) / ( fluid_rbottom + fluid_rmes_min );
        rmes = ( voltageTop - voltageBottom ) / 1000.0 / i;
        value = lround( ( rmes - fluid_rmes_min ) / ( fluid_rmes_max - fluid_rmes_min ) * 100 );
        if ( value < 0 ) {
            value = 0;
        } else if ( value > 100 ) {
            value = 100;
        }
    } else {
        i = 0.0;
        rmes = 0.0;
        value = -1;
        ESP_LOGW( TAG, "Fuel level sensor not connected" );
    }
    *display_value = value;
    if ( rmes_back != nullptr ) {
        *rmes_back = rmes;
    }
    ESP_LOGD( TAG, "convert b=%d t=%d i=%lf rmes=%lf value=%d",
        voltageBottom, voltageTop, i, rmes, *display_value );
}

static void setup_adc_drive_pins() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( PIN_ADC_1_DRIVE );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

static void setup_adc() {
    fluid_user_data data;
    uint8_t adc_index = 0;

    data = {
        .instance = 0,
        .type = N2kft_Fuel,
        .drive_gpio_pin = PIN_ADC_1_DRIVE,
        .capacity = 60,
        .adc_channel_low = static_cast<uint8_t>(adc_index + 0),
        .adc_channel_high = static_cast<uint8_t>(adc_index + 1),
    };
    fluid_data[ fluid_count++ ] = data;
    adc_index += 2;

    adc_add_channel( PIN_ADC_1_LOW - 1, "uzemanyag_l", nullptr, 0, nullptr );
    adc_add_channel( PIN_ADC_1_HIGH - 1, "uzemanyag_h", nullptr, 0, nullptr );
    gpio_set_level( data.drive_gpio_pin, 1 );
    adc_main_oneshot( false );
}

static void setup_water_drive_pins() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT( PIN_WATER_1 ) | BIT( PIN_WATER_2 ) | BIT( PIN_WATER_3 ) | BIT( PIN_WATER_4 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config( &io_conf );

    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pin_bit_mask = BIT( PIN_WATER_0 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[ ] = {
        N2K_PGN_FLUID_LEVEL,
        0
    };

    static const unsigned long ReceiveMessages[ ] = {
        0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
        2100, // N2kVersion
        105, // Manufacturer's product code
        "Fluid level", // Manufacturer's Model ID
        "0.1.0 (2023-03-23)", // Manufacturer's Software version code
        "1.0.0 (2023-03-23)", // Manufacturer's Model version
        "00000001", // Manufacturer's Model serial code
        0, // CertificationLevel
        1 // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    NMEA2000.SetDeviceInformation( n2k_get_device_id(), // Unique number. Use e.g. Serial number.
        150, // Device function=Fluid level
        75, // Device class=Sensor Communication Interface
        N2K_MANUFACTURER_CODE_VARILOG,
        4, // Marine
        iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_adc_fluid_level( int index, tN2kMsg &message, int &deviceIndex ) {
    adc_channel_value_t channel_data_l;
    adc_channel_value_t channel_data_h;

    fluid_user_data *data = fluid_data + index;
    ESP_LOGD( TAG, "adc[%d] driver %d low %d high %d", index, data->drive_gpio_pin, data->adc_channel_low,
        data->adc_channel_high );
    gpio_set_level( data->drive_gpio_pin, 1 );
    // TODO add a bit delay to stabilize voltage
    adc_get_channel_value( data->adc_channel_low, &channel_data_l );
    adc_get_channel_value( data->adc_channel_high, &channel_data_h );
    gpio_set_level( data->drive_gpio_pin, 0 );

    int valueInPercent;
    double r_mes;
    convert_fluid_level( channel_data_l.display_value, channel_data_h.display_value, &valueInPercent, &r_mes );

    ESP_LOGI( TAG, "Channel %d %-10s Raw: %4d Rmes: %lf Display: %5d",
        channel_data_h.channel,
        channel_data_h.name,
        channel_data_h.raw_value,
        r_mes,
        valueInPercent );

    deviceIndex = myDeviceIndex;
    SetN2kFluidLevel( message,
        data->instance,
        (tN2kFluidType) data->type,
        valueInPercent >= 0 ? valueInPercent : N2kDoubleNA, // level
        data->capacity != 0 ? data->capacity : N2kDoubleNA // capacity
    );
    return true;
}

static bool send_water_fluid_level( int index, tN2kMsg &message, int &deviceIndex ) {
    double valueInPercent;

    ESP_LOGD( TAG, "water[%d]", index );

    gpio_set_direction( PIN_WATER_0, GPIO_MODE_OUTPUT );
    gpio_set_level( PIN_WATER_0, 0 );
    // TODO add a bit delay to stabilize voltage

    if ( gpio_get_level( PIN_WATER_4 ) == 0 ) {
        valueInPercent = 100;
    } else if ( gpio_get_level( PIN_WATER_3 ) == 0 ) {
        valueInPercent = 75;
    } else if ( gpio_get_level( PIN_WATER_2 ) == 0 ) {
        valueInPercent = 50;
    } else if ( gpio_get_level( PIN_WATER_1 ) == 0 ) {
        valueInPercent = 25;
    } else {
        valueInPercent = 0;
    }

    gpio_set_direction( PIN_WATER_0, GPIO_MODE_DISABLE );

    ESP_LOGD( TAG, "water level %3d%%", (int) ( valueInPercent * 100 ) );

    int capacity = 85;
    deviceIndex = myDeviceIndex;
    SetN2kFluidLevel( message,
        index - 1,
        N2kft_Water,
        valueInPercent,
        capacity != 0 ? capacity : N2kDoubleNA // capacity
    );

    return true;
}

static bool n2k_send_fluid_level( int index, tN2kMsg &message, int &deviceIndex ) {
    switch ( index ) {
        case 0:
            return send_adc_fluid_level( index, message, deviceIndex );
        case 1:
            return send_water_fluid_level( index, message, deviceIndex );
        default:
            return false;
    }
}

void hajo_fluid_main( int iDev ) {
    myDeviceIndex = iDev;
    setup_adc_drive_pins();
    setup_adc();

    setup_water_drive_pins();

    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_fluid_level, "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL_MS, 250, true );
}
