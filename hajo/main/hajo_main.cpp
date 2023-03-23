extern "C" {
#include "config.h"
#include "display_main.h"
#include "display_meter.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "hajo_adc.h"
#include "n2k_png.h"
#include "lib/adc.h"
}

#include "n2k_sender.h"

#define ESP32_CAN_TX_PIN N2K_GPIO_NUM_TX
#define ESP32_CAN_RX_PIN N2K_GPIO_NUM_RX
#define ESP32_CAN_STANDBY_PIN N2K_GPIO_NUM_STANDBY

#include "NMEA2000_CAN.h"

static const char *LOG = "hajo_main";

// defined by pins 26/21
#define DEVICE_TYPE_GAUGE_DISPLAY 0b111
#define DEVICE_TYPE_BATTERY_MONITOR 0b110
#define DEVICE_TYPE_RESERVED_1 0b01
#define DEVICE_TYPE_RESERVED_0 0b00

static int device_type = 0xff;

static void set_pins() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pin_bit_mask =
            BIT1 | BIT2 | BIT3;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
            // ADC in BIT0 | BIT1 | BIT3 |
            // TWAI BIT4 | BIT5 |
            BIT6 | BIT7 | BIT8 | BIT9 |
            BIT10 | BIT11 | BIT12 | BIT13 | BIT14 |
            BIT15 | BIT16 | BIT17 | BIT18 | BIT19 |
            BIT20 | BIT21 | BIT26 |
            BIT33 | BIT34 | BIT35 | BIT36 | BIT37 | BIT38 | BIT39 |
            BIT40 | BIT41 | BIT42 |
            // USB UART BIT43 | BIT44 |
            BIT45;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );

//    io_conf.intr_type = GPIO_INTR_ANYEDGE;
//    io_conf.mode = GPIO_MODE_INPUT;
//
//    io_conf.pull_down_en = false;
//    io_conf.pull_up_en = true;
//    gpio_config( &io_conf );
}

static bool n2k_send_battery_status( int index, tN2kMsg &message ) {
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

/*
    SetN2kDCStatus( N2kMsg, 1, 1, N2kDCt_Battery, 56, 92, 38500, 0.012 );
    SetN2kBatConf( N2kMsg, 1, N2kDCbt_Gel, N2kDCES_Yes, N2kDCbnv_12v, N2kDCbc_LeadAcid, AhToCoulomb( 420 ), 53,
                   1.251, 75 );
 */

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

static void process_incoming_pgn_battery_status( const tN2kMsg &N2kMsg ) {
    unsigned char instance;
    double voltage;
    double current;
    double temperature;
    unsigned char sid;
    if ( ParseN2kDCBatStatus( N2kMsg, instance, voltage, current, temperature, sid )) {
        ESP_LOGI( LOG, "packet battery status %d = %lf", instance, voltage );
        display_set_value( VOLTAGE, instance, voltage / 100 );
    }
}

static void process_incoming_pgn_fluid_level( const tN2kMsg &N2kMsg ) {
    unsigned char instance;
    tN2kFluidType fluidType;
    double level;
    double capacity;
    if ( ParseN2kFluidLevel( N2kMsg, instance, fluidType, level, capacity )) {
        ESP_LOGI( LOG, "packet fluid level %d/%d = %lf", fluidType, instance, level );
        if ( fluidType == tN2kFluidType::N2kft_Fuel || fluidType == tN2kFluidType::N2kft_FuelGasoline ) {
            display_set_value( FUEL, instance, level );
        } else if ( fluidType == tN2kFluidType::N2kft_Water ) {
            display_set_value( WATER, instance, level );
        }
    }
}

static void determine_device_type() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
            ( 1 << GPIO_NUM_DEVICE_TYPE_0 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_1 ) |
            ( 1 << GPIO_NUM_DEVICE_TYPE_2 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config( &io_conf );
    vTaskDelay(pdMS_TO_TICKS( 10 ));

    device_type = ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_2 ) << 2 ) |
                  ( gpio_get_level( GPIO_NUM_DEVICE_TYPE_1 ) << 1 ) |
                  gpio_get_level( GPIO_NUM_DEVICE_TYPE_0 );
    ESP_LOGI( LOG, "device type %d", device_type );

    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
}

class N2kIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    N2kIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg );
};

void N2kIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
    ESP_LOGI( LOG, "packet pgn %5lx", N2kMsg.PGN );
    switch ( N2kMsg.PGN ) {
        case N2K_PGN_FLUID_LEVEL:
            process_incoming_pgn_fluid_level( N2kMsg );
            break;
        case N2K_PGN_BATTERY_STATUS:
            process_incoming_pgn_battery_status( N2kMsg );
            break;
    }
}

N2kIncomingMessageHandler incomingMessageHandler( &NMEA2000 );

void setup_n2k_display() {
// List here messages your device will transmit.
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_FLUID_LEVEL,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            N2K_PGN_BATTERY_STATUS,
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

    // Set Product information
    NMEA2000.SetProductInformation( &ProductInformation );

    // Set Configuration information
//    NMEA2000.SetConfigurationInformation( "John Doe, john.doe@unknown.com",
//                                          "Just for sample",
//                                          "No real information send to bus" );
    // Set device information
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   170,    // Device function=Battery. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   35,        // Device class=Electrical Generation. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   2046  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );

    uint8_t sourceAddress = 26;
    NMEA2000.SetMode( tNMEA2000::N2km_ListenAndNode, sourceAddress );
    // NMEA2000.EnableForward(false);                      // Disable all msg forwarding to USB (=Serial)
    // NMEA2000.SetN2kCANMsgBufSize(2);                    // For this simple example, limit buffer size to 2, since we are only sending data
    NMEA2000.ExtendTransmitMessages( TransmitMessages );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages );

    NMEA2000.AttachMsgHandler( &incomingMessageHandler ); // NMEA 2000 -> NMEA 0183 conversion
    // NMEA2000.SetMsgHandler(HandleNMEA2000Msg);

    // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
    NMEA2000.SetOnOpen( n2k_onOpen );
    NMEA2000.Open();
}

void setup_n2k_battery() {
// List here messages your device will transmit.
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_BATTERY_STATUS,
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

    // Set Product information
    NMEA2000.SetProductInformation( &ProductInformation );

    // Set Configuration information
//    NMEA2000.SetConfigurationInformation( "John Doe, john.doe@unknown.com",
//                                          "Just for sample",
//                                          "No real information send to bus" );
    // Set device information
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   170,    // Device function=Battery. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   35,        // Device class=Electrical Generation. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   2046  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );

    uint8_t sourceAddress = 0; //25;
    NMEA2000.SetMode( tNMEA2000::N2km_ListenAndNode, sourceAddress );
    // NMEA2000.EnableForward(false);                      // Disable all msg forwarding to USB (=Serial)
    //  NMEA2000.SetN2kCANMsgBufSize(2);                    // For this simple example, limit buffer size to 2, since we are only sending data
    NMEA2000.ExtendTransmitMessages( TransmitMessages );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages );
    // NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
    // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
    NMEA2000.SetOnOpen( n2k_onOpen );
    NMEA2000.Open();
}

static void initialize_twai_driver() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = BIT( N2K_GPIO_NUM_STANDBY );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );
    gpio_set_level( N2K_GPIO_NUM_STANDBY, 0 );
}

extern "C" {
void app_main() {
    determine_device_type();
    initialize_twai_driver();

//    esp_pm_config_esp32s2_t pm_config = {
//            .max_freq_mhz = 80,
//            .min_freq_mhz = 40,
//            .light_sleep_enable = false
//    };
//
//    ESP_ERROR_CHECK( esp_pm_configure( &pm_config ));
//    ESP_ERROR_CHECK( esp_pm_get_configuration( &pm_config ));
//    ESP_LOGI( LOG, "Clock min: %d max: %d", pm_config.min_freq_mhz, pm_config.max_freq_mhz );

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    switch ( device_type ) {
        case DEVICE_TYPE_GAUGE_DISPLAY:
            setup_n2k_display();
            hajo_adc_main( false );
            nk2_register_sender( n2k_send_fluid_level, "fluids", N2K_PGN_FLUID_LEVEL_INTERVAL_MS, 250, true );
            display_main();
//            n2k_register_sender_loopback( process_incoming_pgn );
            break;
        case DEVICE_TYPE_BATTERY_MONITOR:
            setup_n2k_battery();
            hajo_adc_main( true );
            nk2_register_sender( n2k_send_battery_status, "battery", N2K_PGN_BATTERY_STATUS_INTERVAL_MS, 0, true );
            break;
        default:
            // TODO fail
            ESP_LOGE( LOG, "Unhandled device type %c%c%c",
                      device_type & 4 ? '1' : '0',
                      device_type & 2 ? '1' : '0',
                      device_type & 1 ? '1' : '0' );
            break;
    }
}

}
