#include "display_main.h"
#include "esp_log.h"
#include "hajo_display.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"

static const char *LOG = "display";

static void process_incoming_pgn_battery_status( const tN2kMsg &N2kMsg ) {
    N2kDCBatStatusData data;
    if ( ParseN2kDCBatStatus( N2kMsg, data )) {
        ESP_LOGI( LOG, "packet battery status %d: voltage %lf", data.instance, data.voltage );
        int voltageDisplayValue = (int) ( data.voltage * 10 );
        display_set_value( VOLTAGE, data.instance, voltageDisplayValue );
    }
}

static void process_incoming_pgn_dc_detailed_status( const tN2kMsg &N2kMsg ) {
    ParseN2kDCStatusData data;
    if ( ParseN2kDCStatus( N2kMsg, data )) {
        ESP_LOGI( LOG, "packet dc status %d", data.instance );
    }
}

static void process_incoming_pgn_battery_configuration( const tN2kMsg &N2kMsg ) {
    N2kBatConfData data;
    if ( ParseN2kBatConf( N2kMsg, data )) {
        data.batCapacity = CoulombToAh( data.batCapacity );
        ESP_LOGI( LOG, "packet battery conf %d: capacity %lf", data.instance, data.batCapacity );
    }
}

static void process_incoming_pgn_fluid_level( const tN2kMsg &N2kMsg ) {
    N2kFluidLevelData data;
    if ( ParseN2kFluidLevel( N2kMsg, data)) {
        int levelInPercent = (int)(data.level * 100);
        ESP_LOGI( LOG, "packet fluid level %d/%d = %lf", data.fluidType, data.instance, data.level );
        if ( data.fluidType == tN2kFluidType::N2kft_Fuel || data.fluidType == tN2kFluidType::N2kft_FuelGasoline ) {
            display_set_value( FUEL, data.instance, levelInPercent );
        } else if ( data.fluidType == tN2kFluidType::N2kft_Water ) {
            display_set_value( WATER, data.instance, levelInPercent );
        }
    }
}

class DisplayIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    explicit DisplayIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg ) override;
};

void DisplayIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
//    if ( device_type != DEVICE_TYPE_GAUGE_DISPLAY ) {
//        return;
//    }
    ESP_LOGI( LOG, "packet pgn %5lx", N2kMsg.PGN );
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
    }
}

DisplayIncomingMessageHandler* incomingMessageHandler;

void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            0
    };

    static const unsigned long ReceiveMessages[] = {
            N2K_PGN_FLUID_LEVEL,
            N2K_PGN_BATTERY_STATUS,
            N2K_PGN_DC_DETAILED_STATUS,
            N2K_PGN_BATTERY_CONFIGURATION,
            0
    };

    // Set device information
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   120,    // Device function=Display
                                   120,       // Device class=Display
                                   2046, // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    incomingMessageHandler = new DisplayIncomingMessageHandler( &NMEA2000 );
    NMEA2000.AttachMsgHandler( incomingMessageHandler );
}

static void process_incoming_pgn( const tN2kMsg &message ) {
    incomingMessageHandler->HandleMsg( message );
}

void hajo_display_main( int iDev ) {
    setup_n2k_device( iDev );
    display_main();
    n2k_sender_register_loopback( process_incoming_pgn );
}
