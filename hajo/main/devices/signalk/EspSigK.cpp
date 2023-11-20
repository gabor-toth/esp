#include "esp_log.h"
#include "EspSigK.h"
#include "NMEA2000_esp32_stream.h"
#include "cJSON.h"
#include "ssdp.h"
#include "string.h"
#include "wifi_connect.h"
#include "rest_server.h"
#include "ws_server.h"
#include "N2kTimer.h"
#include "lwip/apps/mdns.h"
#include "mdns.h"

static const char* TAG ="signalk";

#if !CONFIG_HTTPD_WS_SUPPORT
#error This example cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

EspSigK sigK;

#if 0
// see https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway


// Server variables
WebServer server( 80 );
WebSocketsServer webSocketServer = WebSocketsServer( 81 );
WebSocketsClient webSocketClient;
#endif

// Simple web page to view deltas
static const char *EspSigKIndexContents = R"foo(
<html>
<head>
  <title>Deltas</title>
  <meta charset="utf-8">
  <script type="text/javascript">
    var WebSocket = WebSocket || MozWebSocket;
    var lastDelta = Date.now();
    var serverUrl = "ws://" + window.location.hostname + ":80/ws";
    var timerId;

    function connect() {
      console.log("Create new connection");
      var connection = new WebSocket(serverUrl);

      connection.onopen = function(evt) {
        clearInterval(timerId);
        console.log("Connected");
        document.getElementById("box").innerHTML = "Connected!";
        document.getElementById("last").innerHTML = "Last: N/A";
      };

      connection.onmessage = function(evt) {
        var msg = JSON.parse(evt.data);
        document.getElementById("box").innerHTML = JSON.stringify(msg, null, 2);
        document.getElementById("last").innerHTML = "Last: " + ((Date.now() - lastDelta)/1000).toFixed(2) + " seconds";
        lastDelta = Date.now();
      };

      connection.onclose = function(evt) {
        console.log("Disconnected");
        document.getElementById("box").innerHTML = "Lost connection";
        timerId = setInterval(function(){
          connect();
        }, 1000);
      };
    }

    connect();

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

EspSigK::EspSigK() {
    printDeltaSerial = false;
    printDebugSerial = false;
//    signalKServerPort = 80;
//    wsClientReconnectInterval = 10000;
}

EspSigK::~EspSigK() {
    stop();
}

//void EspSigK::setServerPort( uint16_t newPort ) {
//    signalKServerPort = newPort;
//}

//void EspSigK::setServerToken( string &token ) {
//    signalKServerToken = token;
//}

void EspSigK::setPrintDeltaSerial( bool v ) {
    printDeltaSerial = v;
}

void EspSigK::setPrintDebugSerial( bool v ) {
    printDebugSerial = v;
}

void EspSigK::setupDiscovery( ) {
    ESP_ERROR_CHECK( ssdp_init());
    ssdp_config_t config = {
            .task_priority       = tskIDLE_PRIORITY + 5,
            .stack_size          = 4096,
            .core_id             = tskNO_AFFINITY,
            .ttl                 = 2,
            .port                = 80,
            .interval            = 1200,
            .mx_max_delay        = 10000,
            .uuid_root           = nullptr,
            .uuid                = nullptr,
            .schema_url          = "description.xml",
            .device_type         = "upnp:rootdevice",
            .friendly_name       = "N2K Gateway",
            .serial_number       = "000000",
            .presentation_url    = "/index.html",
            .manufacturer_name   = "Espressif Systems",
            .manufacturer_url    = "https://www.signalk.org",
            .model_name          = "N2K Gateway",
            .model_url           = "https://www.signalk.org",
            .model_number         = "1.0",
            .model_description    = nullptr,
            .server_name          = hostname.c_str(),
            .services_description = nullptr,
            .icons_description    = nullptr
    };
    ESP_ERROR_CHECK( ssdp_start( &config ));
}

void EspSigK::start( const char *hostname, httpd_handle_t server ) {
    stop();
    if ( printDebugSerial ) {
        ESP_LOGI( TAG,"Starting as host %s", hostname );
    }
    this->hostname = hostname;
    this->http_server = server;

    setupDiscovery();
    setupHTTP();
    setupWebSocket();
}

void EspSigK::stop() {
    ssdp_stop();
}

#if 0
void EspSigK::handle() {
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
}
#endif

/* ******************************************************************** */
/* HTTP                                                                 */
/* ******************************************************************** */

void EspSigK::setupHTTP() {
    ESP_LOGI(TAG, "Registering handlers" );

    httpd_uri_t uri = {
            .uri = "/description.xml",
            .method = HTTP_GET,
            .handler = htmlDescriptionXml,
            .user_ctx = nullptr
    };
    rest_register_uri_handler( http_server, TAG, &uri );

    uri.handler = htmlSignalKEndpoints;
    uri.uri = "/signalk";
    rest_register_uri_handler( http_server, TAG, &uri );
    uri.uri = "/signalk/";
    rest_register_uri_handler( http_server, TAG, &uri );

    uri.handler = htmlIndexContents;
    uri.uri = "/";
    rest_register_uri_handler( http_server, TAG, &uri );
    uri.uri = "/index.html";
    rest_register_uri_handler( http_server, TAG, &uri );

    uri.handler = htmlHandleNotFound;
    uri.uri = "/*";
    rest_register_uri_handler( http_server, TAG, &uri );
}

esp_err_t EspSigK::htmlHandleNotFound(httpd_req_t *r) {
    ESP_LOGW(TAG,"Not found '%s'", r->uri);
    httpd_resp_send_err(r,HTTPD_404_NOT_FOUND, "Not found" );
    return ESP_OK;
}

esp_err_t EspSigK::htmlDescriptionXml(httpd_req_t *r) {
    ESP_LOGI(TAG,"Serving htmlDescriptionXml");
    httpd_resp_set_type( r, "text/xml" );
    const char * schema = get_ssdp_schema_str();
    httpd_resp_send(r, schema, strlen(schema));
    return ESP_OK;
}

esp_err_t EspSigK::htmlIndexContents(httpd_req_t *r) {
    ESP_LOGI(TAG,"Serving htmlIndexContents");
    httpd_resp_set_type( r, HTTPD_TYPE_TEXT );
    httpd_resp_send(r, EspSigKIndexContents, strlen(EspSigKIndexContents));
    return ESP_OK;
}

esp_err_t EspSigK::htmlSignalKEndpoints(httpd_req_t *r) {
    ESP_LOGI(TAG,"Serving htmlSignalKEndpoints");

    esp_netif_t *netif = wifi_get_esp_netif();
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info( netif, &ip_info);

    char wsUrl[64];
    uint32_t ip = ip_info.ip.addr;
    snprintf(wsUrl, sizeof (wsUrl), "ws://%d.%d.%d.%d:81/",(int)((ip>>24)&0xff), (int)((ip>>16)&0xff), (int)((ip>>8)&0xff), (int)(ip&0xff) );

    cJSON *result = cJSON_CreateObject();

    cJSON *endpoints = cJSON_AddObjectToObject(result,"endpoints");
    cJSON *v1 = cJSON_AddObjectToObject(endpoints,"v1" );
    cJSON_AddStringToObject(v1, "version",  "1.alpha1" );
    cJSON_AddStringToObject(v1, "signalk-ws",  wsUrl);

    cJSON *server = cJSON_AddObjectToObject( endpoints,"server" );
    cJSON_AddStringToObject(server, "id",  "ESP-SigKSen");

    char *jsonText = cJSON_Print( result );
    cJSON_Delete(result);

    httpd_resp_set_type( r, HTTPD_TYPE_JSON );
    httpd_resp_send(r, jsonText, strlen(jsonText));
    free(jsonText);
    return ESP_OK;
}

void EspSigK::setupWebSocket() {
//    webSocketServer.begin();
//    webSocketServer.onEvent( webSocketServerEvent );
//    webSocketClient.onEvent( webSocketClientEvent );
//
//    connectWebSocketClient();
}

bool EspSigK::getMDNSService(std::string& host, uint16_t& port ) {
    // get IP address using an mDNS query
    if ( printDebugSerial ) Serial.println( "SIGK: Searching for server via mDNS" );
    mdns_result_t *results = nullptr;
    mdns_query_ptr( "_signalk-ws", "_tcp", 1000, 1, &results );
    if ( results == nullptr) {
        return false;
    }
//    host = results->hostname;
    esp_ip4_addr_t* ip = &results->addr->addr.u_addr.ip4;
    char s[32];
    snprintf(s, sizeof(s), "%d.%d.%d.%d",
            esp_ip4_addr4_16(ip), esp_ip4_addr3_16(ip),
            esp_ip4_addr2_16(ip), esp_ip4_addr1_16(ip) );
    mdns_query_results_free( results );

    host = s;
    port = results->port;
    ESP_LOGI(TAG, "Found SignalK Server via mDNS at %s:%d", signalKServerHost.c_str(), signalKServerPort );
    return true;
}


void EspSigK::connectWebSocketClient() {
    std::string host;
    uint16_t port = 0;
    const char* url = "/signalk/v1/stream?subscribe=none";

    if ( signalKServerHost.length() == 0 ) {
        getMDNSService( host, port );
    } else {
        host = signalKServerHost;
        port = signalKServerPort;
    }

    if ( host.length() == 0 || port == 0 ) {
        ESP_LOGW( TAG, "No server for websocket client" );
        return;
    }
//    if ( signalKServerToken != "" ) {
//        url = url + "&token=" + signalKServerToken;
//    }

    webSocketClient.begin( host, port, url );
    wsClientConnected = true;
}

#if 0
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
#endif



/* ******************************************************************** */
/* SignalK                                                              */
/* ******************************************************************** */

void EspSigK::startDelta( unsigned char source, unsigned long pgn ) {
    deltaSource = source;
    deltaPgn = pgn;
}

void EspSigK::addDeltaValue(const char* path, const char* value) {
    ESP_LOGD(TAG,"add value %s=%s", path, value);
    if ( path== nullptr || value ==nullptr) {
        return;
    }
    Delta delta = Delta(path, value);
    deltas.push_back(delta);
}

void EspSigK::addDeltaValue(const char* path, int value) {
    char buf[16];
    itoa(value,buf,  10);
    addDeltaValue(path, buf);
}

void EspSigK::addDeltaValue( const char* path, double value ) {
    char buf[24];
    snprintf(buf, sizeof (buf), "%lf", value);
    addDeltaValue(path, buf);
}

void EspSigK::addDeltaValue( const char* path, bool value ) {
    addDeltaValue(path, value? "true":"false");
}

void EspSigK::sendDelta() {
    if ( http_server == nullptr) {
        deltas.clear();
        return;
    }

    ESP_LOGD(TAG,"send %d delta values", deltas.size());
    cJSON *result = cJSON_CreateObject();

    //updated array
    cJSON *updatesArr = cJSON_AddArrayToObject( result, "updates" );

    char bufSrc[16], bufPgn[16], bufTimestamp[16];
    itoa(deltaSource, bufSrc, 10);
    utoa(deltaPgn, bufPgn, 10);
    snprintf( bufTimestamp, sizeof(bufTimestamp), "%ld", N2kMillis() );

    cJSON *thisUpdate = cJSON_CreateObject();
    cJSON_AddItemToArray(updatesArr,thisUpdate );
    cJSON *source = cJSON_AddObjectToObject(thisUpdate,"source");
    cJSON_AddStringToObject(source, "label",  hostname.c_str() );
    cJSON_AddStringToObject(source, "type",  "NMEA2000" );
    cJSON_AddStringToObject(source, "src",  bufSrc );
    cJSON_AddStringToObject(source, "pgn", bufPgn  );

    cJSON_AddStringToObject(thisUpdate,"$timestamp", bufTimestamp);

    cJSON *values = cJSON_AddArrayToObject( thisUpdate, "values" );
    for( std::list<Delta>::const_iterator it = deltas.cbegin(); it != deltas.cend(); ++it ) {
        const Delta& delta = *it;
        cJSON *thisValue = cJSON_CreateObject();
        cJSON_AddItemToArray(values,thisValue );
        cJSON_AddStringToObject(thisValue, "path",  delta.path.c_str() );
        cJSON_AddStringToObject(thisValue, "value", delta.value.c_str());
    }

    char *deltaText = cJSON_Print( result );
    cJSON_Delete(result);

    if ( printDeltaSerial ) {
        Serial.println( deltaText );
    }
    wss_server_send_message( http_server, deltaText);
    if ( wsClientConnected ) { // client
//        webSocketClient.sendTXT( deltaText );
    }
    free(deltaText);

    deltas.clear();
}

EspSigK::Delta::Delta(const char *path, const char *value ) {
    this->path= path;
    this->value = value;
}
