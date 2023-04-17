#include "display_main.h"
#include "display_meter.h"
#include "esp_log.h"
#include "lib/nmea2000/n2k_png.h"
#include "n2k_sender.h"
#include "hajo_display.h"

static const char *LOG = "hajo_display";

static void process_incoming_pgn_battery_status( const tN2kMsg &N2kMsg ) {
    unsigned char instance;
    double voltage;
    double current;
    double temperature;
    unsigned char sid;
    if ( ParseN2kDCBatStatus( N2kMsg, instance, voltage, current, temperature, sid )) {
        ESP_LOGI( LOG, "packet battery status %d: voltage %lf", instance, voltage );
        int voltageDisplayValue = (int) ( voltage * 10 );
        display_set_value( VOLTAGE, instance, voltageDisplayValue );
    }
}

static void process_incoming_pgn_dc_detailed_status( const tN2kMsg &N2kMsg ) {
    unsigned char sid;
    unsigned char instance;
    tN2kDCType dcType;
    unsigned char stateOfCharge;
    unsigned char stateOfHealth;
    double timeRemaining;
    double rippleVoltage;
    double capacity;

    if ( ParseN2kDCStatus( N2kMsg, sid, instance, dcType, stateOfCharge, stateOfHealth, timeRemaining, rippleVoltage,
                           capacity )) {
        ESP_LOGI( LOG, "packet dc status %d", instance );
    }
}

static void process_incoming_pgn_battery_configuration( const tN2kMsg &N2kMsg ) {
    unsigned char instance;
    tN2kBatType batType;
    tN2kBatEqSupport supportsEqual;
    tN2kBatNomVolt batNominalVoltage;
    tN2kBatChem batChemistry;
    double batCapacity;
    int8_t batTemperatureCoefficient;
    double peukertExponent;
    int8_t chargeEfficiencyFactor;

    if ( ParseN2kBatConf( N2kMsg, instance, batType, supportsEqual, batNominalVoltage, batChemistry, batCapacity,
                          batTemperatureCoefficient, peukertExponent, chargeEfficiencyFactor )) {

        batCapacity = CoulombToAh( batCapacity );
        ESP_LOGI( LOG, "packet battery conf %d: capacity %lf", instance, batCapacity );
    }
}

static void process_incoming_pgn_fluid_level( const tN2kMsg &N2kMsg ) {
    unsigned char instance;
    tN2kFluidType fluidType;
    double level;
    double capacity;
    if ( ParseN2kFluidLevel( N2kMsg, instance, fluidType, level, capacity )) {
        double levelInPercent = level * 100;
        ESP_LOGI( LOG, "packet fluid level %d/%d = %lf", fluidType, instance, level );
        if ( fluidType == tN2kFluidType::N2kft_Fuel || fluidType == tN2kFluidType::N2kft_FuelGasoline ) {
            display_set_value( FUEL, instance, levelInPercent );
        } else if ( fluidType == tN2kFluidType::N2kft_Water ) {
            display_set_value( WATER, instance, levelInPercent );
        }
    }
}

class N2kIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    N2kIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg );
};

void N2kIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
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

N2kIncomingMessageHandler incomingMessageHandler( &NMEA2000 );

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
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   120,    // Device function=Display
                                   120,       // Device class=Display
                                   2046, // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    NMEA2000.AttachMsgHandler( &incomingMessageHandler );
}

void process_incoming_pgn( const tN2kMsg &message ) {
    incomingMessageHandler.HandleMsg( message );
}

void hajo_display_main( int iDev ) {
    setup_n2k_device( iDev );
    display_main();
    n2k_register_sender_loopback( process_incoming_pgn );
}
