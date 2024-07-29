#include "cmath"
#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"

//static double fluid_u = 3.20;
//static double fluid_rtop = 806;
static double fluid_rbottom = 51.1;
static double fluid_rmes_min = 2;
static double fluid_rmes_max = 180;

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
fluid_user_data fluid_data[MAX_FLUID_COUNT];

//static void convert_fluid_level( uint32_t voltageBottom, uint32_t *display_value, uint32_t *correction ) {
static void
convert_fluid_level( uint32_t voltageBottom, uint32_t voltageTop, uint32_t *display_value, double *rmes_back ) {
    // Rmes=Rtop/(U/Umes-1)-Rbottom
    // 0% = 2 Ohm, 100% = 180 Ohm
    double i = ( voltageBottom / 1000.0 ) / fluid_rbottom;
    double rmes = ( voltageTop - voltageBottom ) / 1000.0 / i;
    uint32_t value;
    if ( rmes <= fluid_rmes_min ) {
        value = 0;
    } else if ( rmes >= fluid_rmes_max ) {
        value = 100;
    } else {
        value = lround( ( rmes - fluid_rmes_min ) / ( fluid_rmes_max - fluid_rmes_min ) * 100 );
    }
    *display_value = value;
//    if ( correction != nullptr ) {
//        *correction = 0;
    if ( rmes_back != nullptr ) {
        *rmes_back = rmes;
    }
    ESP_LOGD( TAG, "convert b=%ld t=%ld i=%lf rmes=%lf value=%ld",
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
            .adc_channel_low =static_cast<uint8_t>(adc_index + 0),
            .adc_channel_high =static_cast<uint8_t>(adc_index + 1),
    };
    fluid_data[ fluid_count++ ] = data;
    adc_index += 2;

    adc_add_channel( PIN_ADC_1_LOW - 1, "uzemanyag_l", nullptr, 0, nullptr );
    adc_add_channel( PIN_ADC_1_HIGH - 1, "uzemanyag_h", nullptr, 0, nullptr );
    gpio_set_level( data.drive_gpio_pin, 1 );
    adc_main( false );
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

    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   150,    // Device function=Fluid level
                                   75,        // Device class=Sensor Communication Interface
                                   2046, // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_adc_fluid_level( int index, tN2kMsg &message ) {
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

    uint32_t valueInPercent;
    double rmes;
    convert_fluid_level( channel_data_l.display_value, channel_data_h.display_value, &valueInPercent, &rmes );

    ESP_LOGD( TAG, "Channel %d %-10s Raw: %4ld Rmes: %lf Display: %5ld",
              channel_data_h.channel,
              channel_data_h.name,
              channel_data_h.raw_value,
              rmes,
              valueInPercent );

    SetN2kFluidLevel( message,
                      data->instance,
                      (tN2kFluidType) data->type,
                      valueInPercent,
                      data->capacity != 0 ? data->capacity : N2kDoubleNA // capacity
    );
    return true;
}

static bool send_water_fluid_level( int index, tN2kMsg &message ) {
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
    SetN2kFluidLevel( message,
                      index - 1,
                      N2kft_Water,
                      valueInPercent,
                      capacity != 0 ? capacity : N2kDoubleNA // capacity
    );

    return true;
}

static bool n2k_send_fluid_level( int index, tN2kMsg &message ) {
    switch ( index ) {
        case 0:
            return send_adc_fluid_level( index, message );
        case 1:
            return send_water_fluid_level( index, message );
        default:
            return false;
    }
}

void hajo_fluid_main( int iDev ) {
    setup_adc_drive_pins();
    setup_adc();

    setup_water_drive_pins();

    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_fluid_level, "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL_MS, 250, true );
}
