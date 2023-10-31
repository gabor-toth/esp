#ifndef EspSigK_H
#define EspSigK_H

#include "esp_http_server.h"
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

//    void setServerHost( string &newServer );
//
//    void setServerPort( uint16_t newPort );
//
//    void setServerToken( string &token );

    void setPrintDeltaSerial( bool v );

    void setPrintDebugSerial( bool v );

    void addDeltaValue( const char *path, const char *value );

    void addDeltaValue( const char *path, int value );

    void addDeltaValue( const char *path, double value );

    void addDeltaValue( const char *path, bool value );

    void sendDelta();

private:
    void setupDiscovery();

    void setupHTTP();

    void setupWebSocket();

//    void connectWebSocketClient();

    static esp_err_t htmlSignalKEndpoints( httpd_req_t *r );

    static esp_err_t htmlHandleNotFound( httpd_req_t *r );

    static esp_err_t htmlIndexContents( httpd_req_t *r );

    static esp_err_t htmlDescriptionXml( httpd_req_t *r );

    httpd_handle_t http_server;
    std::string hostname;
    bool printDeltaSerial;
    bool printDebugSerial;
    std::list<Delta> deltas;

    /*
    const char *signalKServerHost;
    uint16_t signalKServerPort;
    const char *signalKServerToken;
    uint32_t wsClientReconnectInterval;
    bool wsClientConnected;
    uint32_t timerReconnect;
     */
};

//html stuff

//void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
//void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


#endif
