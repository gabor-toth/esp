#include "esp_log.h"
#include "esp_wifi.h"
#include "EspSigK.h"
#include "gyroscope.h"
#include "rest_main.h"
#include "rest_server.h"
#include "wifi_connect.h"
#include "ws_server.h"
#include "n2k/n2k_parser.h"
#include "n2k/n2k_sender.h"

static const char *LOG = "logger";

// see EspSigK.cpp
extern EspSigK sigK;
// see NMEA2000-SignalK-Gateway.cpp
extern void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg ) ;

/*
 * 0x1F801: PGN 129025 - Position, Rapid Update (100msec)
 * 0x1F805: PGN 129029 - GNSS Position Data (1000msec)
 * 0x1F809: PGN 129033 - Time & Date (1000msec)
 * 0ff63
 * 0x1EF00: proprietary fast packet
 * 0x1F10D: PGN 127245 - Rudder
 */
class LoggerIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    explicit LoggerIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg ) override;
};

void process_incoming_pgn_gnss_position_data( const tN2kMsg &msg ) {
    N2kGNSSData data;
    if ( ParseN2kGNSS( msg, data )) {
        ESP_LOGI( LOG, "PGN position data latitude %lf longitude %lf sats %d type %d method %d",
                  data.latitude, data.longitude, data.satellites, data.gnssType, data.gnssMethod );
    }
}

void process_incoming_pgn_local_offset( const tN2kMsg &msg ) {
    N2kLocalOffsetData data;
    if ( ParseN2kLocalOffset( msg, data )) {
        ESP_LOGI( LOG, "PGN local offset days %d seconds %lf offset %d",
                  data.daysSince1970, data.secondsSinceMidnight, data.localOffset );
    }
}

void process_incoming_pgn_rudder( const tN2kMsg &msg ) {
    N2kRudderData data;
    static int counter = 0;
    if ( ++counter < 10 ) {
        return;
    }
    counter = 0;
    if ( ParseN2kRudder( msg, data )) {
        ESP_LOGI( LOG, "PGN rudder pos %lf instance %d",
                  RadToDeg(data.rudderPosition), data.instance );
    }
}

void process_incoming_pgn_proprietary_fast_packet( const tN2kMsg &msg ) {
    int index = 0;
    int vb = msg.Get2ByteUInt(index );
    int manufacturerCode = vb & ((1<<12)-1);
    int industryCode = vb >> 12;
    int proprietaryId = msg.Get2ByteUInt( index );
    int command = msg.GetByte( index );
    ESP_LOGI( LOG, "PGN proprietary manu %04x industry %d proprietaryId %04x/%d command %02d/%d",
              manufacturerCode, industryCode,
              proprietaryId, proprietaryId,
              command, command );
}

// 3b 07     raymarine 73B
// 3b 1f     2x1 reserved
// 3b 9f     marine 4
// f0 81 84  SeaTalk Pilot Mode
// f0 81 86  SeaTalk Keystroke
// f0 81 ae  ?
// proprietary_fast_packet:
// 3b 9f f0 81 ae 02 00 08 00
// 3b 9f f0 81 84 06 00 00 00 00 00 03 06
// N2K_PGN_RAYMARINE_PILOT_MODE:
// 3b 9f 00 00 02 00 01 ff

//I (225529) hajo_logger: PGN local offset days 12326 seconds 76950.000000 offset 32767
//I (225539) hajo_logger: PGN position data latitude 47.580750 longitude 19.060717 sats 4 type 0 method 1

void process_incoming_pgn_dump( const tN2kMsg& msg ) {
    char buf[16*3+1];
    char* p= buf;
    for( int i = 0; i < msg.DataLen; i++, p+= 3) {
        sprintf( p, "%02x ", msg.Data[i]);
    }
    ESP_LOGI( LOG, "PGN %5lx len %2d data %s", msg.PGN, msg.DataLen, buf );
}

void LoggerIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
    sendN2KMessageToSignalK(N2kMsg);

//    ESP_LOGI( LOG, "PGN %5lx len %2d", N2kMsg.PGN, N2kMsg.DataLen );
    switch ( N2kMsg.PGN ) {
//        case N2K_PGN_FLUID_LEVEL:
//            process_incoming_pgn_fluid_level( N2kMsg );
//            break;
//        case N2K_PGN_BATTERY_STATUS:
//            process_incoming_pgn_battery_status( N2kMsg );
//            break;
//        case N2K_PGN_DC_DETAILED_STATUS:
//            process_incoming_pgn_dc_detailed_status( N2kMsg );
//            break;
//        case N2K_PGN_BATTERY_CONFIGURATION:
//            process_incoming_pgn_battery_configuration( N2kMsg );
//            break;
        case N2K_PGN_GNSS_POSITION_DATA:
            process_incoming_pgn_gnss_position_data( N2kMsg );
            break;
        case N2K_PGN_LOCAL_OFFSET:
            process_incoming_pgn_local_offset( N2kMsg );
            break;
        case N2K_PGN_RUDDER:
            process_incoming_pgn_rudder( N2kMsg );
            break;
        case N2K_PGN_PROPRIETARY_FAST_PACKET:
//            process_incoming_pgn_proprietary_fast_packet( N2kMsg );
//            process_incoming_pgn_dump( N2kMsg );
            break;
        case 0xFF63: //N2K_PGN_RAYMARINE_PILOT_MODE:
//            process_incoming_pgn_dump( N2kMsg );
            break;
    }
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            100,                        // Manufacturer's product code
            "Data logger",               // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=Bus Traffic Logger
                                   10,        // Device class=System Tools
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );
//    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
//                                   131,    // Device function=NMEA 2000 to Analog Gateway
//                                   25,        // Inter/Intranetwork Device
//                                   2047,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
//                                   4,       // Marine
//                                   iDev
//    );

    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
//    NMEA2000.SetMode( tNMEA2000::N2km_ListenOnly, NodeAddress );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    LoggerIncomingMessageHandler *incomingMessageHandler = new LoggerIncomingMessageHandler( &NMEA2000 );
    NMEA2000.AttachMsgHandler( incomingMessageHandler );
}

static esp_err_t rest_register_handlers( httpd_handle_t server, rest_server_context_t * ) {
//    sigK.setPrintDebugSerial(true);       // Default false, causes debug messages to be printed to Serial (connecting etc)
//    sigK.setPrintDeltaSerial(false);       // Default false, prints deltas to Serial.
    //sigK.setServerHost("192.168.0.20");    // Optional. Sets the ip of the SignalKServer to connect to. If not set we try to discover server with mDNS
    //sigK.setServerPort(80);                // If manually setting host, this sets the port for the signalK Server (default 80);
    //sigK.setServerToken("secret"); // if you have security enabled in node server, it wont accept deltas unles you auth
    wss_register_handler(server);
    sigK.start("n2k-gw",server);

    return ESP_OK;
}

void hajo_logger_main( int iDev ) {
    // test_sdcard();
    setup_n2k_device( iDev );
    setup_gyroscope();
    rest_init_before_wifi();
    rest_server_main( rest_register_handlers, wss_open_fd, wss_close_fd);
    ESP_ERROR_CHECK( wifi_connect() );
    ESP_LOGI(LOG,"init finished");
}
