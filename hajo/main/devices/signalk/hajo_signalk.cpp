#include "hajo_signalk.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "EspSigK.h"
#include "http/http_events.h"
#include "http/http_main.h"
#include "http/http_server.h"
#include "wifi/wifi_main.h"
#include "ws_server.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"

static const char* TAG = "hajo_signalk";

// see NMEA2000-SignalK-Gateway.cpp
extern void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg );

class SignalkIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    explicit SignalkIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg ) override;
};

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
            DEVICE_NAME,               // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   131,    // Device function=NMEA 2000 to Analog Gateway
                                   25,        // Inter/Intranetwork Device
                                   2047,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
//    NMEA2000.SetMode( tNMEA2000::N2km_ListenOnly, NodeAddress );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    ESP_LOGI(TAG,"Registering SignalkIncomingMessageHandler");
    SignalkIncomingMessageHandler *incomingMessageHandler = new SignalkIncomingMessageHandler( &NMEA2000 );
    NMEA2000.AttachMsgHandler( incomingMessageHandler );
}

static void signalk_start(  void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    http_server_server_event_data * data = static_cast<http_server_server_event_data *>(event_data);
    ESP_LOGI( TAG, "signalk_start" );
    sigK.start( DEVICE_NAME, "n2k-gw", data->hd, data->ssid );
}

static void signalk_stop(  void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "signalk_stop" );
    sigK.stop();
}

static void signalk_register() {
    ESP_LOGI( TAG, "signalk_register" );
    ESP_ERROR_CHECK(
            esp_event_handler_register(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START, &signalk_start, nullptr ));
    ESP_ERROR_CHECK(
            esp_event_handler_register(HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOP, &signalk_stop, nullptr ));
}

static void process_incoming_pgn( const tN2kMsg &N2kMsg ) {
    sendN2KMessageToSignalK( N2kMsg );
}

void SignalkIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
    sendN2KMessageToSignalK( N2kMsg );
}

void hajo_signalk_main( int iDev ) {
    setup_n2k_device( iDev );
    n2k_sender_register_loopback( process_incoming_pgn );

    discovery_register();
    wss_register();
    signalk_register();
    http_server_main();

    ESP_ERROR_CHECK( wifi_connect());
//    ESP_LOGI( TAG, "init finished" );
}
