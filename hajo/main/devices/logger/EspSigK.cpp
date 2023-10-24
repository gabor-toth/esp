#include "EspSigK.h"
#include "NMEA2000_esp32_stream.h"
#include "cJSON.h"

#if 0
// see https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway


// Server variables
WebServer server( 80 );
WebSocketsServer webSocketServer = WebSocketsServer( 81 );
WebSocketsClient webSocketClient;
#endif

static bool printDeltaSerial;
static bool printDebugSerial;

static bool wsClientConnected;

// Simple web page to view deltas
static const char *EspSigKIndexContents = R"foo(
<html>
<head>
  <title>Deltas</title>
  <meta charset="utf-8">
  <script type="text/javascript">
    var WebSocket = WebSocket || MozWebSocket;
    var lastDelta = Date.now();
    var serverUrl = "ws://" + window.location.hostname + ":81";

    connection = new WebSocket(serverUrl);

    connection.onopen = function(evt) {
      console.log("Connected!");
      document.getElementById("box").innerHTML = "Connected!";
      document.getElementById("last").innerHTML = "Last: N/A";
    };

    connection.onmessage = function(evt) {
      var msg = JSON.parse(evt.data);
      document.getElementById("box").innerHTML = JSON.stringify(msg, null, 2);
      document.getElementById("last").innerHTML = "Last: " + ((Date.now() - lastDelta)/1000).toFixed(2) + " seconds";
      lastDelta = Date.now();
    };

    setInterval(function(){
      document.getElementById("age").innerHTML = "Age: " + ((Date.now() - lastDelta)/1000).toFixed(1) + " seconds";
    }, 50);
  </script>
</head>
<body>
  <h3>Last Delta</h3>
  <pre width="100%" height="50%" id="box">Not Connected yet</pre>
  <div id="last"></div>
  <div id="age"></div>
</body>
</html>
)foo";









/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* Constructor/Settings                                                 */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
EspSigK::EspSigK( string& hostname, string& ssid, string& ssidPass ) {
    myHostname = hostname;
    mySSID = ssid;
    mySSIDPass = ssidPass;


//    webSocketServer = WebSocketsServer( 81 );
    wsClientConnected = false;

    signalKServerHost = "";
    signalKServerPort = 80;
    signalKServerToken = "";

    printDeltaSerial = false;
    printDebugSerial = false;

    wsClientReconnectInterval = 10000;

    timerReconnect = millis();

    idxDeltaValues = 0; // init deltas
    for ( uint8_t i = 0; i < MAX_DELTA_VALUES; i++ ) {
        deltaPaths[ i ] = "";
        deltaValues[ i ] = "";
    }
}

void EspSigK::setServerHost( string& newServer ) {
    signalKServerHost = newServer;
}

void EspSigK::setServerPort( uint16_t newPort ) {
    signalKServerPort = newPort;
}

void EspSigK::setServerToken( string& token ) {
    signalKServerToken = token;
}

void EspSigK::setPrintDeltaSerial( bool v ) {
    printDeltaSerial = v;
}

void EspSigK::setPrintDebugSerial( bool v ) {
    printDebugSerial = v;
}

/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* Setup                                                                */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */


void EspSigK::connectWifi() {
    if ( printDebugSerial ) Serial.print( "SIGK: Connecting to Wifi" );
//    WiFi.begin( mySSID.c_str(), mySSIDPass.c_str());
//    while ( WiFi.status() != WL_CONNECTED ) {
//        delay( 200 );
//        if ( printDebugSerial ) Serial.print( "." );
//    }
//    if ( printDebugSerial ) {
//        Serial.println();
//        Serial.print( "SIGK: Connected to Wifi!. IP: " );
//        Serial.println( WiFi.localIP());
//    }
}

void EspSigK::setupDiscovery() {
//    if ( !MDNS.begin( myHostname.c_str())) {             // Start the mDNS responder for esp8266.local
//        if ( printDebugSerial ) Serial.println( "SIGK: Error setting up MDNS responder!" );
//    } else {
//        MDNS.addService( "http", "tcp", 80 );
//        if ( printDebugSerial ) {
//            Serial.print( "SIGK: mDNS responder started at " );
//            Serial.print( myHostname.c_str() );
//            Serial.println( "" );
//        }
//    }
//
//    if ( printDebugSerial ) Serial.println( "SIGK: Starting SSDP..." );
//    SSDP.setSchemaURL( "description.xml" );
//    SSDP.setHTTPPort( 80 );
//    SSDP.setName( myHostname );
//    SSDP.setSerialNumber( "12345" );
//    SSDP.setURL( "index.html" );
//    SSDP.setModelName( "WifiSensorNode" );
//    SSDP.setModelNumber( "12345" );
//    SSDP.setModelURL( "http://www.signalk.org" );
//    SSDP.setManufacturer( "SigK" );
//    SSDP.setManufacturerURL( "http://www.signalk.org" );
//    SSDP.setDeviceType( "upnp:rootdevice" );
//    SSDP.begin();
}

#if 0
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* Run                                                                  */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
void EspSigK::begin() {
    if ( printDebugSerial ) {
        Serial.print( "SIGK: Starting as host: " );
        Serial.println( myHostname );
    }

    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
       would try to act as both a client and an access-point and could cause
       network-issues with your other WiFi-devices on your WiFi-network. */
//    WiFi.mode( WIFI_STA );
    connectWifi();
    setupDiscovery();
    setupHTTP();
    setupWebSocket();
}

void EspSigK::handle() {
//    yield(); //let the ESP do whatever it needs to...

    //Timers
    uint32_t currentMilis = millis();
    //Overflow handle
    if ( timerReconnect > currentMilis ) { timerReconnect = currentMilis; }

    // reconnect timers, make sure wifi connected and websocket connected
    if (( timerReconnect + wsClientReconnectInterval ) < currentMilis ) {
        if ( WiFi.status() != WL_CONNECTED ) {
            connectWifi();
        }
        if ( !wsClientConnected ) {
            connectWebSocketClient();
        }
        timerReconnect = currentMilis;
    }


    //HTTP
    server.handleClient();
    //WS
    webSocketServer.loop();
    if ( wsClientConnected ) {
        webSocketClient.loop();
    }
}

// our delay function will let stuff like websocket/http etc run instead of blocking
void EspSigK::safeDelay( unsigned long ms ) {
    uint32_t start = millis();

    while ( millis() < ( start + ms )) {
        handle();
    }
}


/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* HTTP                                                                 */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
void EspSigK::setupHTTP() {
    if ( printDebugSerial ) Serial.println( "SIGK: Starting HTTP Server" );
    server.onNotFound( htmlHandleNotFound );

    server.on( "/description.xml", HTTP_GET, []() { SSDP.schema( server.client()); } );
    server.on( "/signalk", HTTP_GET, htmlSignalKEndpoints );
    server.on( "/signalk/", HTTP_GET, htmlSignalKEndpoints );

    server.on( "/", []() {
        server.send( 200, "text/html", EspSigKIndexContents );
    } );
    server.on( "/index.html", []() {
        server.send( 200, "text/html", EspSigKIndexContents );
    } );
    server.begin();
}

void EspSigK::htmlHandleNotFound() {
    server.send( 404, "text/plain",
                 "404: Not found" ); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void EspSigK::htmlSignalKEndpoints() {
    IPAddress ip;
    //DynamicJsonBuffer jsonBuffer;
    DynamicJsonDocument jsonBuffer( 300 );
    char response[2048];
    string wsURL;
    ip = WiFi.localIP();

    //JsonObject& json = jsonBuffer.createObject();

    JsonObject json = jsonBuffer.to<JsonObject>();
    string ipString = string( ip[ 0 ] );
    for ( uint8_t octet = 1; octet < 4; ++octet ) {
        ipString += '.' + string( ip[ octet ] );
    }


    wsURL = "ws://" + ipString + ":81/";

    JsonObject endpoints = json.createNestedObject( "endpoints" );
    JsonObject v1 = endpoints.createNestedObject( "v1" );
    v1[ "version" ] = "1.alpha1";
    v1[ "signalk-ws" ] = wsURL;
    JsonObject serverInfo = json.createNestedObject( "server" );
    serverInfo[ "id" ] = "ESP-SigKSen";
    //json.printTo(response);
    serializeJson( jsonBuffer, response );
    server.send( 200, "application/json", response );
}

/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* Websocket                                                            */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
void EspSigK::setupWebSocket() {

    webSocketServer.begin();
    webSocketServer.onEvent( webSocketServerEvent );
    webSocketClient.onEvent( webSocketClientEvent );

    connectWebSocketClient();
}

bool EspSigK::getMDNSService( string &host, uint16_t &port ) {
    // get IP address using an mDNS query
    if ( printDebugSerial ) Serial.println( "SIGK: Searching for server via mDNS" );
    int n = MDNS.queryService( "signalk-ws", "tcp" );
    if ( n == 0 ) {
        // no service found
        return false;
    } else {
        host = MDNS.IP( 0 ).toString();
        port = MDNS.port( 0 );
        if ( printDebugSerial ) {
            Serial.print( "SIGK: Found SignalK Server via mDNS at: " );
            Serial.print( host.c_str() );
            Serial.print( ":" );
            Serial.println( port );
        }
        return true;
    }
}


void EspSigK::connectWebSocketClient() {
    string host = "";
    uint16_t port = 80;
    string url = "/signalk/v1/stream?subscribe=none";

    if ( signalKServerHost.length() == 0 ) {
        getMDNSService( host, port );
    } else {
        host = signalKServerHost;
        port = signalKServerPort;
    }

    if (( host.length() > 0 ) &&
        ( port > 0 )) {
        if ( printDebugSerial ) Serial.println( "SIGK: Websocket client attempting to connect!" );
    } else {
        if ( printDebugSerial ) Serial.println( "SIGK: No server for websocket client" );
        return;
    }
    if ( signalKServerToken != "" ) {
        url = url + "&token=" + signalKServerToken;
    }

    webSocketClient.begin( host, port, url );
    wsClientConnected = true;
}

void webSocketClientEvent( WStype_t type, uint8_t *payload, size_t length ) {
    switch ( type ) {
        case WStype_DISCONNECTED: {
            wsClientConnected = false;
            if ( printDebugSerial ) Serial.println( "SIGK: Websocket Client Disconnected!" );
            break;
        }
        case WStype_CONNECTED: {
            wsClientConnected = true;
            if ( printDebugSerial ) Serial.printf( "SIGK: Client Connected to url: %s\n", payload );
            break;
        }
        case WStype_TEXT:
            //Serial.printf("[WSc] get text: %s\n", payload);
            //receiveDelta(payload);
            break;
        case WStype_BIN:
            //Serial.printf("[WSc] get binary length: %u\n", length);
            //hexdump(payload, length);
            break;
    }
}

void webSocketServerEvent( uint8_t num, WStype_t type, uint8_t *payload, size_t length ) {
    switch ( type ) {
        case WStype_DISCONNECTED: {
            if ( printDebugSerial ) Serial.printf( "SIGK: Websocket Server [%u] Disconnected!\n", num );
            break;
        }
        case WStype_CONNECTED: {
            IPAddress ip = webSocketServer.remoteIP( num );
            if ( printDebugSerial )
                Serial.printf( "SIGK: Websocket Server [%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[ 0 ],
                               ip[ 1 ], ip[ 2 ], ip[ 3 ], payload );
            break;
        }
        case WStype_TEXT:
            break;
        case WStype_BIN:
            break;
    }
}



/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* SignalK                                                              */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */

void EspSigK::addDeltaValue( string& path, string& value ) {
    deltaPaths[ idxDeltaValues ] = path;
    deltaValues[ idxDeltaValues ] = value;
    idxDeltaValues++;
}

void EspSigK::addDeltaValue( string& path, int value ) {
    string v = string( value );
    deltaPaths[ idxDeltaValues ] = path;
    deltaValues[ idxDeltaValues ] = v;
    idxDeltaValues++;
}

void EspSigK::addDeltaValue( string& path, double value ) {
    string v = string( value );
    deltaPaths[ idxDeltaValues ] = path;
    deltaValues[ idxDeltaValues ] = v;
    idxDeltaValues++;
}

void EspSigK::addDeltaValue( string& path, bool value ) {
    string v;
    if ( value ) { v = "true"; } else { v = "false"; }
    deltaPaths[ idxDeltaValues ] = path;
    deltaValues[ idxDeltaValues ] = v;
    idxDeltaValues++;
}

void EspSigK::sendDelta( string& path, string& value ) {
    addDeltaValue( path, value );
    sendDelta();
}

void EspSigK::sendDelta( string& path, int value ) {
    addDeltaValue( path, value );
    sendDelta();
}

void EspSigK::sendDelta( string& path, double value ) {
    addDeltaValue( path, value );
    sendDelta();
}

void EspSigK::sendDelta( string& path, bool value ) {
    addDeltaValue( path, value );
    sendDelta();
}

void EspSigK::sendDelta() {
    cJSON *delta = cJSON_CreateObject();

    //updated array
    cJSON *updatesArr = cJSON_AddArrayToObject( delta, "updates" );

    cJSON *thisUpdate = cJSON_CreateObject();
    cJSON_AddItemToArray(updatesArr,thisUpdate );
    cJSON *source = cJSON_AddObjectToObject(thisUpdate,"source");
    cJSON_AddStringToObject(source, "label",  "ESP" );
    cJSON_AddStringToObject(source, "src",  myHostname.c_str() );

    cJSON *values = cJSON_AddArrayToObject( thisUpdate, "values" );
    for ( uint8_t i = 0; i < idxDeltaValues; i++ ) {
        cJSON *thisValue = cJSON_CreateObject();
        cJSON_AddItemToArray(values,thisValue );
        cJSON_AddStringToObject(thisValue, "path",  deltaPaths[ i ].c_str() );
        cJSON_AddStringToObject(thisValue, "value", deltaValues[ i ].c_str()); // serialized(?)
    }

    char *deltaText = cJSON_Print( delta );

    if ( printDeltaSerial ) {
        Serial.println( deltaText );
    }
    webSocketServer.broadcastTXT( deltaText );
    if ( wsClientConnected ) { // client
        webSocketClient.sendTXT( deltaText );
    }
    free(deltaText);

    //reset delta info
    idxDeltaValues = 0; // init deltas
    for ( uint8_t i = 0; i < MAX_DELTA_VALUES; i++ ) {
        deltaPaths[ i ] = "";
        deltaValues[ i ] = "";
    }
}
#endif
