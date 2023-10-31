#ifndef EspSigK_H
#define EspSigK_H

#include "esp_http_server.h"
#include <list>
#include <string>

//extern "C" {
//  #include "user_interface.h"
//}

//#include <string>

//#include <WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
//#include <ESPmDNS.h>
//
//#include <ESP32SSDP.h>
//#include <WebServer.h> //Local WebServer used to serve the configuration portal
//
//
//#include <ArduinoJson.h>     //https://github.com/bblanchon/ArduinoJson
//#include <WebSocketsServer.h>
//#include <WebSocketsClient.h>

//using namespace std;
//
class EspSigK {
private:
    class Delta {
    public:
        Delta( const char *path, char *value );

        std::string path;
        std::string value;
    };


public:
    EspSigK();

    ~EspSigK();

    void start( const char *hostname, httpd_handle_t server );

    void stop();

//    EspSigK( string &hostname, string &ssid, string &ssidPass );
//
//    void setServerHost( string &newServer );
//
//    void setServerPort( uint16_t newPort );
//
//    void setServerToken( string &token );

    void setPrintDeltaSerial( bool v );

    void setPrintDebugSerial( bool v );

//    void handle();
//
//    void safeDelay( unsigned long ms );

    void addDeltaValue( const char *path, char *value );

    void addDeltaValue( const char *path, int value );

//    void addDeltaValue( const char *path, double value );

//    void addDeltaValue( const char *path, bool value );

    void sendDelta();

//    void sendDelta( string &path, string &value );
//
//    void sendDelta( string &path, int value );
//
//    void sendDelta( string &path, double value );
//
//    void sendDelta( string &path, bool value );

private:
    void setupDiscovery();

    void setupHTTP();

//    void setupWebSocket();
//
//    bool getMDNSService( string &host, uint16_t &port );
//
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
};

//html stuff

//void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
//void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


#endif
