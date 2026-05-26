#include "display_main.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hajo_display.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"
#include "n2k/N2kVarilog.h"

//static const char *TAG = "display";

static void process_incoming_pgn_battery_status( const tN2kMsg &N2kMsg ) {
    N2kDCBatStatusData data;
    if ( ParseN2kDCBatStatus( N2kMsg, data ) ) {
        //        ESP_LOGI( LOG, "packet battery status %d: voltage %lf", data.instance, data.voltage );
        int voltageDisplayValue = (int) ( data.voltage * 10 );
        display_set_value( VOLTAGE, data.instance, voltageDisplayValue );
    }
}

static void process_incoming_pgn_dc_detailed_status( const tN2kMsg &N2kMsg ) {
    ParseN2kDCStatusData data;
    if ( ParseN2kDCStatus( N2kMsg, data ) ) {
        //        ESP_LOGI( LOG, "packet dc status %d", data.instance );
    }
}

static void process_incoming_pgn_battery_configuration( const tN2kMsg &N2kMsg ) {
    N2kBatConfData data;
    if ( ParseN2kBatConf( N2kMsg, data ) ) {
        data.batCapacity = CoulombToAh( data.batCapacity );
        //        ESP_LOGI( LOG, "packet battery conf %d: capacity %lf", data.instance, data.batCapacity );
    }
}

static void process_incoming_pgn_fluid_level( const tN2kMsg &N2kMsg ) {
    N2kFluidLevelData data;
    if ( ParseN2kFluidLevel( N2kMsg, data ) ) {
        int levelInPercent = (int) data.level;
        //        ESP_LOGI( LOG, "packet fluid level %d/%d = %lf", data.fluidType, data.instance, data.level );
        if ( data.fluidType == tN2kFluidType::N2kft_Fuel || data.fluidType == tN2kFluidType::N2kft_FuelGasoline ) {
            display_set_value( FUEL, data.instance, levelInPercent );
        } else if ( data.fluidType == tN2kFluidType::N2kft_Water ) {
            display_set_value( WATER, data.instance, levelInPercent );
        }
    }
}

//static void process_incoming_pgn( const tN2kMsg &message, int deviceIndex ) {
static void process_incoming_pgn( const tN2kMsg &N2kMsg ) {
    //ESP_LOGI( TAG, "packet received with pgn %5lx", N2kMsg.PGN );
    //    if ( device_type != DEVICE_TYPE_DISPLAY ) {
    //        return;
    //    }
    switch ( N2kMsg.PGN ) {
        case N2K_PGN_FLUID_LEVEL:
            process_incoming_pgn_fluid_level( N2kMsg );
            break;
        case N2K_PGN_BATTERY_STATUS:
            process_incoming_pgn_battery_status( N2kMsg );
            break;
        case N2K_PGN_DC_DETAILED_STATUS:
            process_incoming_pgn_dc_detailed_status( N2kMsg );
            break;
        case N2K_PGN_BATTERY_CONFIGURATION:
            process_incoming_pgn_battery_configuration( N2kMsg );
            break;
        case 0x1ef00: // proprietary fast packet
        case 0x1f112: // PGN 127250 - Vessel Heading
        case 0x1f10d: // PGN 127245 - Rudder
        case 0x0ff4f: // ?
            break;
        default:
            //            ESP_LOGI( LOG, "new packet pgn %5lx", N2kMsg.PGN );
            break;
    }
}

void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[ ] = {
        0
    };

    static const unsigned long ReceiveMessages[ ] = {
        N2K_PGN_FLUID_LEVEL,
        N2K_PGN_BATTERY_STATUS,
        N2K_PGN_DC_DETAILED_STATUS,
        N2K_PGN_BATTERY_CONFIGURATION,
        0
    };

    // Set device information
    NMEA2000.SetDeviceInformation( n2k_get_device_id(), // Unique number. Use e.g. Serial number.
        120, // Device function=Display
        120, // Device class=Display
        N2K_MANUFACTURER_CODE_VARILOG,
        4, // Marine
        iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    NMEA2000.AttachMsgHandler( new N2kIncomingMessageHandler( &NMEA2000, process_incoming_pgn ) );
}

static void setup_can_standby_pin() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT( GPIO_NUM_10 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config( &io_conf );
}

static void change_can_standby_pin( bool standby ) {
    gpio_set_level( GPIO_NUM_10, standby );
}

void hajo_display_main( int iDev ) {
    setup_n2k_device( iDev );
    setup_can_standby_pin();
    change_can_standby_pin( false );
    display_main();
    n2k_sender_register_loopback( process_incoming_pgn );
}
