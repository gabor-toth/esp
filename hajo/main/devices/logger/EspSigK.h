#ifndef EspSigK_H
#define EspSigK_H

//extern "C" {
//  #include "user_interface.h"
//}

#include <string>

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

#define MAX_DELTA_VALUES 10

using namespace std;

class EspSigK {
protected:
    string myHostname;
    string mySSID;
    string mySSIDPass;

    string signalKServerHost;
    uint16_t signalKServerPort;
    string signalKServerToken;

    string deltaPaths[MAX_DELTA_VALUES];
    string deltaValues[MAX_DELTA_VALUES];
    uint8_t idxDeltaValues;

    uint32_t wsClientReconnectInterval;

    uint32_t timerReconnect;


public:
    EspSigK( string &hostname, string &ssid, string &ssidPass );

    void setServerHost( string &newServer );

    void setServerPort( uint16_t newPort );

    void setServerToken( string &token );

    void setPrintDeltaSerial( bool v );

    void setPrintDebugSerial( bool v );


    void begin();

    void handle();

    void safeDelay( unsigned long ms );

    void addDeltaValue( string &path, string &value );

    void addDeltaValue( string &path, int value );

    void addDeltaValue( string &path, double value );

    void addDeltaValue( string &path, bool value );

    void sendDelta();

    void sendDelta( string &path, string &value );

    void sendDelta( string &path, int value );

    void sendDelta( string &path, double value );

    void sendDelta( string &path, bool value );

private:
    void connectWifi();

    void setupDiscovery();

    void setupHTTP();

    void setupWebSocket();

    bool getMDNSService( string &host, uint16_t &port );

    void connectWebSocketClient();

    static void htmlSignalKEndpoints();

    static void htmlHandleNotFound();
};

//html stuff

//void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
//void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


#endif
