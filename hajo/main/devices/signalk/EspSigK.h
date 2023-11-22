#ifndef EspSigK_H
#define EspSigK_H

/**
 * see https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway
 */

#include "esp_http_server.h"
#include "esp_websocket_client.h"
#include "freertos/timers.h"
#include <list>
#include <string>

class EspSigK {
private:
    class Delta {
    public:
        Delta( const char *path, const char *value );

        std::string path;
        std::string value;
    };


public:
    EspSigK();

    ~EspSigK();

    void start( const char *hostname, httpd_handle_t server );

    void stop();

    void setSignalkServer( const char *host, uint16_t port, const char *token = nullptr );

    void setPrintDeltaSerial( bool v );

    void setPrintDebugSerial( bool v );

    void startDelta( unsigned char source, unsigned long pgn );

    void addDeltaValue( const char *path, const char *value );

    void addDeltaValue( const char *path, int value );

    void addDeltaValue( const char *path, double value );

    void addDeltaValue( const char *path, bool value );

    void sendDelta();

private:
    void setupDiscovery();

    void setupHTTP();

    void setupWebSocket();

    bool connectWebSocketClient();

    bool getMDNSService( std::string &host, uint16_t &port );

    static esp_err_t htmlSignalKEndpoints( httpd_req_t *r );

    static esp_err_t htmlHandleNotFound( httpd_req_t *r );

    static esp_err_t htmlIndexContents( httpd_req_t *r );

    static esp_err_t htmlDescriptionXml( httpd_req_t *r );

    void onWebSocketClientEvent( esp_event_base_t event_base, int32_t event_id, void *event_data );

    static void webSocketClientEventHandler( void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                             void *event_data );

    _Noreturn static void taskWsClientConnect( void *arg );

    static void triggerWsClientConnect( TimerHandle_t timer );

    httpd_handle_t http_server;
    std::string hostname;
    bool printDeltaSerial;
    bool printDebugSerial;
    std::list<Delta> deltas;
    unsigned char deltaSource;
    unsigned long deltaPgn;

    std::string signalKServerHost;
    std::string signalKServerToken;
    uint16_t signalKServerPort;
    esp_websocket_client_handle_t wsClientHandle;
    uint32_t wsClientReconnectInterval;
    static QueueHandle_t event_queue;
    TimerHandle_t timer;

    bool wsClientConnected;
    /*
    uint32_t timerReconnect;
     */
};

extern EspSigK sigK;

//html stuff

//void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
//void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


#endif
