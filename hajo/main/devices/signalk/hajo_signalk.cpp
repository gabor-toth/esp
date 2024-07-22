#include "hajo_signalk.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "EspSigK.h"
#include "http/http_events.h"
#include "http/http_discovery.h"
#include "http/http_server.h"
#include "signalk_rest.h"
#include "wifi/wifi_main.h"
#include "ws_server.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"
#include "NMEA2000-SignalK-Gateway.h"

static const char *TAG = "hajo_signalk";

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
    ESP_LOGI( TAG, "Registering SignalkIncomingMessageHandler" );
    SignalkIncomingMessageHandler *incomingMessageHandler = new SignalkIncomingMessageHandler( &NMEA2000 );
    NMEA2000.AttachMsgHandler( incomingMessageHandler );
}

static void signalk_start( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    http_server_server_event_data *data = static_cast<http_server_server_event_data *>(event_data);
    ESP_LOGI( TAG, "signalk_start" );
    sigK.start( DEVICE_NAME, "n2k-gw", data->hd, data->ssid );
}

static void signalk_stop( void *dummy, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    ESP_LOGI( TAG, "signalk_stop start" );
    sigK.stop();
    ESP_LOGI( TAG, "signalk_stop end" );
}

static void signalk_register() {
    ESP_LOGI( TAG, "signalk_register" );
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_START,
                                        &signalk_start, nullptr ) );
    ESP_ERROR_CHECK(
            esp_event_handler_register( HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_SERVER_STOPPING,
                                        &signalk_stop, nullptr ) );
    sigK.init();
}

const uart_port_t uart_num = UART_NUM_1;

class SerialN2kStream : public N2kStream {
    int read() override {
        return 0;
    }

    int peek() override {
        return 0;
    }

    size_t write( const uint8_t *data, size_t size ) override {
        return uart_write_bytes( uart_num, data, size );
    }
};

SerialN2kStream serialN2kStream = {};

static void process_incoming_pgn( const tN2kMsg &N2kMsg ) {
    N2kMsg.SendInActisenseFormat(&serialN2kStream);
    sendN2KMessageToSignalK( N2kMsg );
}

void SignalkIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
    N2kMsg.SendInActisenseFormat(&serialN2kStream);
    sendN2KMessageToSignalK( N2kMsg );
}

static void setup_signalk_uart() {
    uart_config_t uart_config = {
//            .baud_rate = 921600,
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, GPIO_NUM_33, GPIO_NUM_21, GPIO_NUM_NC, GPIO_NUM_NC));
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size,
                                        uart_buffer_size, 10, &uart_queue, 0));
}

void hajo_signalk_main( int iDev ) {
    setup_n2k_device( iDev );
    n2k_sender_register_loopback( process_incoming_pgn );

    setup_signalk_uart();

    discovery_register();
    wss_register();
    signalk_register();
    signalk_rest_register();
    http_server_main(0);

    ESP_ERROR_CHECK( wifi_main() );
}
