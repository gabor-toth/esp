#include "esp_log.h"
#include "esp_netif_ip_addr.h"
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
#include "logger.h"
#include "adc.h"

static const char* TAG ="signalk";
static const char* TAG_WSCLIENT ="wsclient";

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

QueueHandle_t EspSigK::event_queue = nullptr;

EspSigK::EspSigK() {
    printDeltaSerial = false;
    printDebugSerial = false;
//    signalKServerHost = "10.128.65.180";
    signalKServerPort = 3000;
    wsClientReconnectInterval = 10000;
}

EspSigK::~EspSigK() {
    stop();
}

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
    ESP_LOGI( TAG,"Starting as host %s", hostname );
    this->hostname = hostname;
    this->http_server = server;

//    setupDiscovery();
    setupHTTP();
    setupWebSocket();
}

void EspSigK::stop() {
    ssdp_stop();
    if ( wsClientHandle ) {
        esp_websocket_client_stop(wsClientHandle);
        esp_websocket_client_destroy(wsClientHandle);
        wsClientHandle = nullptr;
    }
    // TODO guard this
    if ( timer ) {
        xTimerStop( timer, portMAX_DELAY );
        xTimerDelete( timer, portMAX_DELAY );
        timer = nullptr;
    }
}

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
    snprintf(wsUrl, sizeof (wsUrl), "ws://" IPSTR ":81/", IP2STR(&ip_info.ip));

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

void EspSigK::triggerWsClientConnect(TimerHandle_t timer) {
    (void)timer;

    int dummy = 0;
    xQueueSend(event_queue, &dummy, portMAX_DELAY);
}

_Noreturn void EspSigK::taskWsClientConnect( void *arg ) {
    (void) arg;

    EspSigK* _this = static_cast<EspSigK *>(arg);
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( event_queue, &dummy, portMAX_DELAY ) ) {
            if ( !_this->connectWebSocketClient() ) {
                xTimerStart( _this->timer, portMAX_DELAY );
            }
        }
    }
}

void EspSigK::setupWebSocket() {
    if ( !event_queue ) {
        event_queue  = xQueueCreate( 10, sizeof( int ));
        if ( xTaskCreate( taskWsClientConnect, TAG_WSCLIENT, 3072, this, tskIDLE_PRIORITY, nullptr ) != pdTRUE ) {
            ESP_LOGE(TAG_WSCLIENT, "Error create websocket task");
        }
        timer = xTimerCreate(
                TAG,
                pdMS_TO_TICKS(wsClientReconnectInterval),
                0,
                nullptr,
                triggerWsClientConnect );
    }
    triggerWsClientConnect(timer);
}

bool EspSigK::getMDNSService(std::string& host, uint16_t& port ) {
    ESP_LOGI( TAG_WSCLIENT, "mDNS start lookup" );

    mdns_result_t *results = nullptr;
    esp_err_t err = mdns_query_ptr( "_signalk-ws", "_tcp", 3000, 20, &results );
    if (err) {
        ESP_LOGE(TAG_WSCLIENT, "mDNS query failed: %s", esp_err_to_name(err));
        return false;
    }
    if ( results == nullptr) {
        ESP_LOGI( TAG_WSCLIENT, "mDNS no service found" );
        return false;
    }
    char s[32];
    mdns_result_t *result = results;
    ESP_LOGI( TAG_WSCLIENT, "mDNS dump services" );
    while ( result != nullptr)    {
        esp_ip4_addr_t *ip = &result->addr->addr.u_addr.ip4;
        snprintf( s, sizeof( s ), IPSTR, IP2STR(ip));
        ESP_LOGI(TAG_WSCLIENT,"- host %s ip %s instance %s",
                 result->hostname ? result->hostname : "null", s, result->instance_name ? result->instance_name : "null");
        result = result->next;
    }
    mdns_query_results_free( results );

    host = s;
    port = results->port;
    ESP_LOGI(TAG_WSCLIENT, "Found SignalK Server via mDNS at %s:%d", host.c_str(), port );
    return true;
}

void EspSigK::setSignalkServer( const char* host, uint16_t port, const char* token ) {
    signalKServerHost = host;
    signalKServerPort = port;
    signalKServerToken = token;
}

void EspSigK::onWebSocketClientEvent( esp_event_base_t event_base, int32_t event_id, void* event_data ) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch ( event_id ) {
        case WEBSOCKET_EVENT_CONNECTED:
            wsClientConnected = true;
            ESP_LOGI(TAG_WSCLIENT,"event connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            wsClientConnected = false;
            ESP_LOGI(TAG_WSCLIENT,"event disconnected");
            triggerWsClientConnect( nullptr);
            break;
        case WEBSOCKET_EVENT_ERROR:
            wsClientConnected = false;
            ESP_LOGW(TAG_WSCLIENT,"event error type %d", data->error_handle.error_type);
            // transport_error=ESP_OK, tls_error_code=0, tls_flags=0, errno=119
//            if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI( TAG_WSCLIENT,"reported from esp-tls %d", data->error_handle.esp_tls_last_esp_err);
                ESP_LOGI( TAG_WSCLIENT,"reported from tls stack %d", data->error_handle.esp_tls_stack_err);
                ESP_LOGI( TAG_WSCLIENT,"captured as transport's socket errno %d",  data->error_handle.esp_transport_sock_errno);
//            }
            triggerWsClientConnect( nullptr);
            break;
        case WEBSOCKET_EVENT_DATA: {
            if ( data->op_code & 0x08 ) {
                // Websocket control frame
                break;
            }
            if ( data->op_code == 1 ) {
                char *buf = static_cast<char *>(malloc( data->data_len + 1 ));
                memcpy( buf, data->data_ptr, data->data_len );
                buf[ data->data_len ] = 0;
                ESP_LOGI( TAG_WSCLIENT, "received %s", buf );
                free( buf );
            } else if ( data->op_code == 1 ) {
                ESP_LOGI( TAG_WSCLIENT, "received binary len %d", data->data_len );
            }
            break;
        }
        default:
            ESP_LOGI( TAG_WSCLIENT, "ignored event %ld", event_id );
            break;
    }
}

void EspSigK::webSocketClientEventHandler( void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data ) {
    ((EspSigK*)event_handler_arg)->onWebSocketClientEvent(event_base, event_id, event_data);
}

bool EspSigK::connectWebSocketClient() {
    std::string host;
    uint16_t port = 0;

    ESP_LOGI(TAG_WSCLIENT, "starting");

    if ( !getMDNSService( host, port ) && signalKServerHost.length() != 0) {
        host = signalKServerHost;
        port = signalKServerPort;
        ESP_LOGW( TAG_WSCLIENT, "using configured host %s", host.c_str() );
    }

    if ( host.length() == 0 || port == 0 ) {
        ESP_LOGW( TAG_WSCLIENT, "no server found" );
        return false;
    }

    const esp_websocket_client_config_t ws_cfg = {
            .host = host.c_str(),
            .port = port,
            .path = "/signalk/v1/stream?subscribe=none",
            .disable_auto_reconnect = true,
            .transport = WEBSOCKET_TRANSPORT_OVER_TCP,
            .keep_alive_enable = true,
            .reconnect_timeout_ms = 10000,
            .network_timeout_ms = 10000,
    };

    // https://docs.espressif.com/projects/esp-idf/en/v4.1/api-reference/protocols/esp_websocket_client.html

    esp_err_t err;
    wsClientHandle = esp_websocket_client_init(&ws_cfg);
    if (wsClientHandle == nullptr) {
        ESP_LOGE(TAG_WSCLIENT, "esp_websocket_client_init failed");
        return false;
    }
    err = esp_websocket_client_start(wsClientHandle);
    if (err) {
        ESP_LOGE(TAG_WSCLIENT, "esp_websocket_client_start failed with %s", esp_err_to_name(err));
        esp_websocket_client_destroy(wsClientHandle);
        return false;
    }

    err = esp_websocket_register_events( wsClientHandle, WEBSOCKET_EVENT_ANY, webSocketClientEventHandler, this);
    if (err) {
        ESP_LOGE(TAG_WSCLIENT, "failed to register event WEBSOCKET_EVENT_CONNECTED: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG_WSCLIENT, "started");
    return true;
}

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
        ESP_LOGD(TAG, "%s", deltaText );
    }
    wss_server_send_message( http_server, deltaText);
    if ( wsClientConnected ) {
        if ( esp_websocket_client_send_text(wsClientHandle, deltaText, strlen(deltaText), 10) == ESP_FAIL ) {
            ESP_LOGE(TAG_WSCLIENT,"error sending delta");
        }
    }
    free(deltaText);

    deltas.clear();
}

EspSigK::Delta::Delta(const char *path, const char *value ) {
    this->path= path;
    this->value = value;
}
