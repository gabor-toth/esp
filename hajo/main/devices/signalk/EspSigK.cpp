#include <cstring>
#include "EspSigK.h"
#include "N2kTimer.h"
#include "NMEA2000_esp32_stream.h"
#include "adc.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif_ip_addr.h"
#include "nvs_main.h"
#include "mdns.h"
#include "http/http_server.h"
#include "ssdp.h"
#include "wifi/wifi_main.h"
#include "ws_server.h"

#define NVS_KEY_TOKEN "signalk.token"
#define NVS_KEY_UUID "signalk.uuid"
#define NVS_KEY_REQUEST_ID "sk.requestId"

#define TIMER_WS_CONNECT    1
#define TIMER_TOKEN_CHECK   2

static const char *TAG = "signalk";
static const char *TAG_WSCLIENT = "sk_wsclient";
static const char *TAG_HTTPCLIENT = "sk_httpclient";

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
    signalKServerPort = 80;
    wsClientReconnectInterval = 12000;
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

void EspSigK::setupDiscovery() {
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

void EspSigK::start( const char *deviceName, const char *hostname, httpd_handle_t server, const char* wifi_ssid ) {
//    signalKServerToken.clear();
//    saveSetting(NVS_KEY_TOKEN, signalKServerToken);

    stop();
    ESP_LOGI( TAG, "Starting as host %s on network %s", hostname, wifi_ssid );
    this->hostname = hostname;
    this->http_server = server;
    this->deviceName = deviceName;

    if ( strcmp( wifi_ssid, "TothKiss") == 0 ) {
        signalKServerHost = "192.168.72.180";
    } else if ( strcmp( wifi_ssid, "P92WG_E") == 0 || strcmp( wifi_ssid, "TGA") == 0 ) {
        // signalKServerHost = "10.128.65.20";
        signalKServerHost = "79.122.115.7";
    }

//    setupDiscovery();
    setupHTTP();
    setupWebSocket();
}

void EspSigK::stop() {
    http_server = nullptr;
//    ssdp_stop();
    stopWsClient();
    if ( wsClientConnectTimer ) {
//        xTimerStop( wsClientConnectTimer, portMAX_DELAY );
        xTimerDelete( wsClientConnectTimer, portMAX_DELAY );
        wsClientConnectTimer = nullptr;
    }
    if ( pendingTokenTimer ) {
//        xTimerStop( pendingTokenTimer, portMAX_DELAY );
        xTimerDelete( pendingTokenTimer, portMAX_DELAY );
        pendingTokenTimer = nullptr;
    }
    if ( event_queue ) {
        vQueueDelete( event_queue);
        event_queue = nullptr;
    }
    if ( wsTask ) {
        vTaskDelete(wsTask);
        wsTask = nullptr;
    }
}

/* ******************************************************************** */
/* HTTP                                                                 */
/* ******************************************************************** */

void EspSigK::setupHTTP() {
    ESP_LOGI( TAG, "Registering handlers" );

    httpd_uri_t uri = {
            .uri = "/description.xml",
            .method = HTTP_GET,
            .handler = htmlDescriptionXml,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr,
    };
    http_register_uri_handler(http_server, TAG, &uri);

    uri.handler = htmlSignalKEndpoints;
    uri.uri = "/signalk";
    http_register_uri_handler(http_server, TAG, &uri);
    uri.uri = "/signalk/";
    http_register_uri_handler(http_server, TAG, &uri);

    uri.handler = htmlIndexContents;
    uri.uri = "/";
    http_register_uri_handler(http_server, TAG, &uri);
    uri.uri = "/index.html";
    http_register_uri_handler(http_server, TAG, &uri);
}

esp_err_t EspSigK::htmlDescriptionXml( httpd_req_t *r ) {
    ESP_LOGI( TAG, "Serving htmlDescriptionXml" );
    httpd_resp_set_type( r, "text/xml" );
    const char *schema = get_ssdp_schema_str();
    httpd_resp_send( r, schema, strlen( schema ));
    return ESP_OK;
}

esp_err_t EspSigK::htmlIndexContents( httpd_req_t *r ) {
    ESP_LOGI( TAG, "Serving htmlIndexContents" );
    httpd_resp_set_type( r, HTTPD_TYPE_TEXT );
    httpd_resp_send( r, EspSigKIndexContents, strlen( EspSigKIndexContents ));
    return ESP_OK;
}

esp_err_t EspSigK::htmlSignalKEndpoints( httpd_req_t *r ) {
    ESP_LOGI( TAG, "Serving htmlSignalKEndpoints" );

    esp_netif_t *netif = wifi_get_esp_netif();
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info( netif, &ip_info );

    char wsUrl[64];
    snprintf( wsUrl, sizeof( wsUrl ), "ws://" IPSTR ":81/", IP2STR( &ip_info.ip ));

    cJSON *result = cJSON_CreateObject();

    cJSON *endpoints = cJSON_AddObjectToObject( result, "endpoints" );
    cJSON *v1 = cJSON_AddObjectToObject( endpoints, "v1" );
    cJSON_AddStringToObject( v1, "version", "1.alpha1" );
    cJSON_AddStringToObject( v1, "signalk-ws", wsUrl );

    cJSON *server = cJSON_AddObjectToObject( endpoints, "server" );
    cJSON_AddStringToObject( server, "id", "ESP-SigKSen" );

    char *jsonText = cJSON_Print( result );
    cJSON_Delete( result );

    httpd_resp_set_type( r, HTTPD_TYPE_JSON );
    httpd_resp_send( r, jsonText, strlen( jsonText ));
    free( jsonText );
    return ESP_OK;
}

void EspSigK::triggerWsClientConnect( ) {
    uint32_t timerId = TIMER_WS_CONNECT;
    xQueueSend( event_queue, &timerId, portMAX_DELAY );
}

void EspSigK::timerCallback( TimerHandle_t timer ) {
    uint32_t timerId = (uint32_t)pvTimerGetTimerID( timer );
    xQueueSend( event_queue, &timerId, portMAX_DELAY );
}

void EspSigK::taskWsClientConnect( void *arg ) {
    (void) arg;

    EspSigK *_this = static_cast<EspSigK *>(arg);
    for ( ;; ) {
        uint32_t timerId;
        if ( xQueueReceive( event_queue, &timerId, portMAX_DELAY )) {
            if ( timerId == TIMER_TOKEN_CHECK ) {
                ESP_LOGI(TAG,"On pendingTokenTimer");
                _this->onTokenTimer();
            } else if ( timerId == TIMER_WS_CONNECT ) {
                ESP_LOGI(TAG,"On wsClientConnectTimer");
                if ( !_this->connectWsClient()) {
                    _this ->startWsClientConnectTimer( );
                }
            }
        }
    }
}

void EspSigK::setupWebSocket() {
    if ( !event_queue ) {
        event_queue = xQueueCreate( 10, sizeof( int ));
        if ( xTaskCreate( taskWsClientConnect, TAG_WSCLIENT, 3072, this, tskIDLE_PRIORITY, &wsTask ) != pdTRUE) {
            ESP_LOGE( TAG_WSCLIENT, "Error create websocket task" );
        }
        wsClientConnectTimer = xTimerCreate(
                TAG,
                pdMS_TO_TICKS( wsClientReconnectInterval ),
                0,
                (void *) TIMER_WS_CONNECT,
                timerCallback );
        pendingTokenTimer = xTimerCreate(
                TAG,
                pdMS_TO_TICKS( 5000 ),
                0,
                (void *) TIMER_TOKEN_CHECK,
                timerCallback );
    }
    triggerWsClientConnect();
}

bool EspSigK::findMDNSService() {
    if ( signalKServerHost.empty()) {
        ESP_LOGI( TAG_WSCLIENT, "mDNS start lookup" );

        mdns_result_t *results = nullptr;
        esp_err_t err = mdns_query_ptr( "_signalk-ws", "_tcp", 3000, 20, &results );
        if ( err ) {
            ESP_LOGE( TAG_WSCLIENT, "mDNS query failed: %s", esp_err_to_name( err ));
            return false;
        }
        if ( results == nullptr ) {
            ESP_LOGI( TAG_WSCLIENT, "mDNS no service found" );
            return false;
        }
        char s[32];
        mdns_result_t *result = results;
        ESP_LOGI( TAG_WSCLIENT, "mDNS dump services" );
        while ( result != nullptr ) {
            if ( result->addr != nullptr ) {
                esp_ip4_addr_t *ip = &result->addr->addr.u_addr.ip4;
                snprintf( s, sizeof( s ), IPSTR, IP2STR( ip ));
            } else {
                strncpy( s, "null", sizeof( s ));
            }
            ESP_LOGI( TAG_WSCLIENT, "- host %s ip %s instance %s",
                      result->hostname ? result->hostname : "null",
                      s,
                      result->instance_name ? result->instance_name : "null" );
            result = result->next;
        }
        mdns_query_results_free( results );

        signalKServerHost = s;
        signalKServerPort = results->port;
    }

    ESP_LOGI( TAG_WSCLIENT, "Found SignalK server %s:%d via mDNS", signalKServerHost.c_str(), signalKServerPort );
    return true;
}

void EspSigK::onWebSocketClientEvent( esp_event_base_t event_base, int32_t event_id, void *event_data ) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *) event_data;
    switch ( event_id ) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI( TAG_WSCLIENT, "event connected" );
            onClientConnected();
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI( TAG_WSCLIENT, "event disconnected" );
            onClientDisconnected();
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW( TAG_WSCLIENT, "event error type %d esp_tls_last_esp_err=%d esp_tls_stack_err=%d esp_transport_sock_errno=%d esp_ws_handshake_status_code=%d",
                      data->error_handle.error_type,
                      data->error_handle.esp_tls_last_esp_err,
                      data->error_handle.esp_tls_stack_err,
                      data->error_handle.esp_transport_sock_errno,
                      data->error_handle.esp_ws_handshake_status_code );
            wsClientConnected = false;
            if ( data->error_handle.esp_ws_handshake_status_code == 401 ) {
                ESP_LOGW(TAG_WSCLIENT,"Token invalid, requesting new one");
                signalKServerToken.clear();
                saveSetting(NVS_KEY_TOKEN, signalKServerToken);
                clearRequestId();
                triggerWsClientConnect();
            } else {
                startWsClientConnectTimer();
            }
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
                onClientTextReceived( buf );
                free( buf );
            } else if ( data->op_code == 2 ) {
                ESP_LOGI( TAG_WSCLIENT, "received binary len %d", data->data_len );
            }
            break;
        }
        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGW( TAG_WSCLIENT, "event closed" );
            onClientDisconnected();
            break;
        case WEBSOCKET_EVENT_BEFORE_CONNECT:
            //ESP_LOGI( TAG_WSCLIENT, "event before_connect" );
            break;
        default:
            ESP_LOGI( TAG_WSCLIENT, "ignored event %ld", event_id );
            break;
    }
}

void EspSigK::webSocketClientEventHandler( void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                           void *event_data ) {
    ((EspSigK *) event_handler_arg )->onWebSocketClientEvent( event_base, event_id, event_data );
}

bool EspSigK::connectWsClient() {
    stopWsClient();

    ESP_LOGI( TAG_WSCLIENT, "starting" );

    if ( !signalKServerHost.empty() ) {
        ESP_LOGI( TAG_WSCLIENT, "Using SignalK server %s:%d", signalKServerHost.c_str(), signalKServerPort );
    } else if ( !findMDNSService()) {
        return false;
    }

    if ( !loadSetting( NVS_KEY_UUID, signalKUuid )) {
        setUuid();
    }
    if ( signalKServerToken.empty() && !loadSetting( NVS_KEY_TOKEN, signalKServerToken )) {
        sendAccessRequest();
        // do not retry to connect ws until we get the token
        return true;
    }
    authHeader = "Authorization: Bearer "+ signalKServerToken + "\r\n";

    const esp_websocket_client_config_t ws_cfg = {
            .uri = nullptr,
            .host = signalKServerHost.c_str(),
            .port = signalKServerPort,
            .username = nullptr,
            .password = nullptr,
            .path = "/signalk/v1/stream?subscribe=none",
            .disable_auto_reconnect = false, // 'true' causes a race condition in esp_websocket_client_task()
            .user_context = nullptr,
            .task_prio = 0,
            .task_name = nullptr,
            .task_stack = 0,
            .buffer_size = 0,
            .cert_pem = nullptr,
            .cert_len = 0,
            .client_cert = nullptr,
            .client_cert_len = 0,
            .client_key  = nullptr,
            .client_key_len = 0,
            .transport = WEBSOCKET_TRANSPORT_OVER_TCP,
            .subprotocol = nullptr,
            .user_agent = nullptr,
            .headers = authHeader.empty() ? nullptr : authHeader.c_str(),
            .pingpong_timeout_sec = 0,
            .disable_pingpong_discon = false,
            .use_global_ca_store = false,
            .crt_bundle_attach = nullptr,
            .skip_cert_common_name_check = false,
            .keep_alive_enable = true,
            .keep_alive_idle = 0,
            .keep_alive_interval = 0,
            .keep_alive_count = 0,
            .reconnect_timeout_ms = 10000,
            .network_timeout_ms = 10000,
            .ping_interval_sec = 0,
            .if_name = nullptr,
    };

    // https://docs.espressif.com/projects/esp-idf/en/v4.1/api-reference/protocols/esp_websocket_client.html

    esp_err_t err;
    wsClientHandle = esp_websocket_client_init( &ws_cfg );
    ESP_LOGI( TAG_WSCLIENT, "starting %p", wsClientHandle );
    if ( wsClientHandle == nullptr ) {
        ESP_LOGE( TAG_WSCLIENT, "esp_websocket_client_init failed" );
        return false;
    }

    err = esp_websocket_register_events( wsClientHandle, WEBSOCKET_EVENT_ANY, webSocketClientEventHandler, this );
    if ( err ) {
        ESP_LOGE( TAG_WSCLIENT, "failed to register event WEBSOCKET_EVENT_CONNECTED: %s", esp_err_to_name( err ));
    }

    err = esp_websocket_client_start( wsClientHandle );
    if ( err ) {
        ESP_LOGE( TAG_WSCLIENT, "esp_websocket_client_start failed with %s", esp_err_to_name( err ));
        esp_websocket_client_destroy( wsClientHandle );
        wsClientHandle = nullptr;
        return false;
    }

    ESP_LOGI( TAG_WSCLIENT, "started %p", wsClientHandle );
    return true;
}

void EspSigK::stopWsClient() {
    ESP_LOGI( TAG_WSCLIENT, "stopping %p", wsClientHandle );
    if ( wsClientHandle != nullptr ) {
        ESP_ERROR_CHECK(esp_websocket_client_destroy( wsClientHandle ) );
        wsClientHandle = nullptr;
    }
    wsClientConnected = false;
    ESP_LOGI( TAG_WSCLIENT, "stopped" );
}

/* ******************************************************************** */
/* SignalK                                                              */
/* ******************************************************************** */

DeltaSet::DeltaSet( unsigned char source, unsigned long pgn ) {
    this->source = source;
    this->pgn = pgn;
}

void DeltaSet::addValue( const char *path, const char *value ) {
    ESP_LOGD( TAG, "add value %s=%s", path, value );
    if ( path == nullptr || value == nullptr ) {
        return;
    }
    DeltaValue delta = DeltaValue( path, value );
    deltas.push_back( delta );
}

void DeltaSet::addValue( const char *path, int value ) {
    char buf[16];
    itoa( value, buf, 10 );
    addValue( path, buf );
}

void DeltaSet::addValue( const char *path, double value ) {
    char buf[24];
    snprintf( buf, sizeof( buf ), "%lf", value );
    addValue( path, buf );
}

void DeltaSet::addValue( const char *path, bool value ) {
    addValue( path, value ? "true" : "false" );
}

void EspSigK::sendDeltaSet( DeltaSet &deltaSet ) {
    if ( http_server == nullptr ) {
        return;
    }

    const std::list<DeltaValue> &deltas = deltaSet.getDeltas();

    ESP_LOGD( TAG, "send %d delta values", deltas.size());
    cJSON *result = cJSON_CreateObject();

    //updated array
    cJSON *updatesArr = cJSON_AddArrayToObject( result, "updates" );

    char bufSrc[16], bufPgn[16], bufTimestamp[16];
    itoa( deltaSet.getSource(), bufSrc, 10 );
    utoa( deltaSet.getPgn(), bufPgn, 10 );
    snprintf( bufTimestamp, sizeof( bufTimestamp ), "%ld", N2kMillis());

    cJSON *thisUpdate = cJSON_CreateObject();
    cJSON_AddItemToArray( updatesArr, thisUpdate );
    cJSON *source = cJSON_AddObjectToObject( thisUpdate, "source" );
    cJSON_AddStringToObject( source, "label", hostname.c_str());
    cJSON_AddStringToObject( source, "type", "NMEA2000" );
    cJSON_AddStringToObject( source, "src", bufSrc );
    cJSON_AddStringToObject( source, "pgn", bufPgn );

    cJSON_AddStringToObject( thisUpdate, "$timestamp", bufTimestamp );

    cJSON *values = cJSON_AddArrayToObject( thisUpdate, "values" );
    for ( std::list<DeltaValue>::const_iterator it = deltas.cbegin(); it != deltas.cend(); ++it ) {
        const DeltaValue &delta = *it;
        cJSON *thisValue = cJSON_CreateObject();
        cJSON_AddItemToArray( values, thisValue );
        cJSON_AddStringToObject( thisValue, "path", delta.path.c_str());
        if (delta.value.length() != 0 ) {
            cJSON_AddStringToObject( thisValue, "value", delta.value.c_str() );
        } else {
            cJSON_AddItemReferenceToObject( thisValue, "value", nullptr );
        }
    }

    char *deltaText = cJSON_PrintUnformatted( result );
    cJSON_Delete( result );

    if ( printDeltaSerial ) {
        ESP_LOGI( TAG, "%s", deltaText );
    }
    wss_server_send_message( http_server, deltaText );
    if ( wsClientConnected ) {
        if ( esp_websocket_client_send_text( wsClientHandle, deltaText, strlen( deltaText ), 10 ) == ESP_FAIL ) {
            ESP_LOGE( TAG_WSCLIENT, "error sending delta" );
            onClientDisconnected();
        }
    }
    free( deltaText );
}

void DeltaSet::send( EspSigK &espSigk ) {
    espSigk.sendDeltaSet( *this );
}

DeltaValue::DeltaValue( const char *path, const char *value ) {
    this->path = path;
    this->value = value;
}

void EspSigK::onClientConnected() {
}

void EspSigK::onClientDisconnected() {
    wsClientConnected = false;
    startWsClientConnectTimer();
}

void EspSigK::onClientTextReceived( const char *buf ) {
    bool processed = false;

    if ( *buf == '{' ) {
        // cJSON_ParseWithLengthOpts(buf, 0, nullptr, false);
        cJSON *result = cJSON_Parse( buf );
        cJSON *item;
        if ( cJSON_GetObjectItem( result, "version" ) != nullptr ) {
            processFrameHello( result );
            processed = true;
        } else if ( ( item = cJSON_GetObjectItem( result, "message" ) ) != nullptr ) {
            ESP_LOGW( TAG_WSCLIENT, "message: %s", cJSON_GetStringValue(item) );
            processed = true;
        }
        cJSON_Delete( result );
    }
    if ( !processed ) {
        ESP_LOGW( TAG_WSCLIENT, "skipped message %s", buf );
    }
}

void EspSigK::processFrameHello( cJSON *o ) {
    /*
     * {
     *  "name":"signalk-server",
     *  "version":"2.4.1",
     *  "self":"vessels.urn:mrn:signalk:uuid:469c08d9-267c-4ad0-b469-c1acba7665b7",
     *  "roles":["master","main"],
     *  "timestamp":"2023-12-04T15:01:56.618Z"
     * }
     */
    ESP_LOGI( TAG_WSCLIENT, "connected to %s version %s",
              cJSON_GetStringValue( cJSON_GetObjectItem( o, "name" )),
              cJSON_GetStringValue( cJSON_GetObjectItem( o, "version" ))
    );
    wsClientConnected = true;
}

bool EspSigK::loadSetting( const char *name, std::string &value ) {
    uint32_t nvs_handle = nvs_open_storage();
    if ( nvs_handle == 0) {
        return false;
    }
    char *s = nvs_read_string( nvs_handle, name );
    if ( s != nullptr ) {
        value = s;
        ESP_LOGI( TAG, "Loaded %s=%s", name, s );
        free( s );
    } else {
        value.clear();
        ESP_LOGI( TAG, "No %s yet", name );
    }
    nvs_close_storage( nvs_handle );
    return !value.empty();
}

void EspSigK::saveSetting( const char *name, const std::string &value ) {
    ESP_LOGI( TAG, "Save %s", name );
    uint32_t nvs_handle = nvs_open_storage();
    if ( nvs_handle == 0) {
        return;
    }
    nvs_write_string( nvs_handle, name, value.c_str());
    nvs_close_storage( nvs_handle );
}

void EspSigK::setUuid() {
    uint8_t chipid[6];
    esp_efuse_mac_get_default( chipid );
    char uuid[40];
    snprintf( uuid, sizeof( uuid ), "b4c46698-92cd-11ee-b9d1-%02x%02x%02x%02x%02x%02x",
              chipid[ 0 ], chipid[ 1 ], chipid[ 2 ], chipid[ 3 ], chipid[ 4 ], chipid[ 5 ] );
    signalKUuid = uuid;
    saveSetting( NVS_KEY_UUID, signalKUuid );
}

 void EspSigK::sendAndHandleAccessRequest() {
    cJSON *result = nullptr;
    char *state;
    int statusCode;
    char *buffer;

    ESP_LOGI( TAG_HTTPCLIENT, "request" );
    esp_err_t  err = httpClientData.send();
    if ( err != ESP_OK ) {
        ESP_LOGE( TAG_HTTPCLIENT, "HTTP POST request failed: %s", esp_err_to_name( err ));
        goto retry;
    }

     buffer = httpClientData.getBuffer();
     if ( buffer == nullptr || httpClientData.getBufferLength() == 0 ) {
        ESP_LOGW( TAG_HTTPCLIENT, "no data received, check log" );
        goto retry;
    }

    ESP_LOGI( TAG_HTTPCLIENT, R"(response %d '%s')", httpClientData.getStatusCode(), buffer );
    result = cJSON_Parse( buffer );
    state = cJSON_GetStringValue( cJSON_GetObjectItem( result, "state" ) );
    if ( httpClientData.getStatusCode() == 404 || httpClientData.getStatusCode() == 500 ) {
        // 500 Unable to check request: not found
        ESP_LOGW( TAG_HTTPCLIENT, "access request not found, firing new one" );
        clearRequestIdAndRequestNew();
    } else if ( state == nullptr ) {
        ESP_LOGW( TAG_HTTPCLIENT, "unable to parse response as JSON" );
        prepareAccessRequest();
        startPendingTokenTimer();
    } else if ( strcmp( state, "PENDING" ) == 0 ) {
        if ( !pendingTokenState ) {
            char *requestId = cJSON_GetStringValue( cJSON_GetObjectItem( result, "requestId" ));
            if ( requestId == nullptr ) {
                ESP_LOGW( TAG_HTTPCLIENT, "no requestId in response, can't wait on it" );
                goto retry;
            }
            saveSetting(NVS_KEY_REQUEST_ID, requestId );
            prepareCheckAccessRequest( requestId );
            pendingTokenState = true;
        }
        startPendingTokenTimer();
    } else if ( strcmp( state, "COMPLETED" ) == 0 ) {
        statusCode = (int)cJSON_GetNumberValue( cJSON_GetObjectItem( result, "statusCode" ));
        if ( statusCode ==  400 ) {
            // response {
            //   "state":"COMPLETED",
            //   "requestId":"52ec0b7b-0c2f-4417-b5c5-8dcf1fcaafe9",
            //   "statusCode":400,
            //   "message":"A device with clientId 'b4c46698-92cd-11ee-b9d1-70041dfbbcf6' has already requested access",
            //   "href":"/signalk/v1/requests/52ec0b7b-0c2f-4417-b5c5-8dcf1fcaafe9",
            //   "ip":"::ffff:192.168.72.182"
            // }

            char *message = cJSON_GetStringValue( cJSON_GetObjectItem( result, "message" ));
            if ( message != nullptr ) {
                ESP_LOGW( TAG_HTTPCLIENT, "%s", message );
            }
            clearRequestIdAndRequestNew();
        } else {
            cJSON *accessRequestObject = cJSON_GetObjectItem( result, "accessRequest" );
            char *permission = cJSON_GetStringValue( cJSON_GetObjectItem( accessRequestObject, "permission" ));
            if ( strcmp( permission, "APPROVED" ) == 0 ) {
                //  "accessRequest": {"permission": "APPROVED","token": "eyJhbGciOiJIUzI1NiIs...BAP8bt3tNBT1WiIttm3qM",...}
                signalKServerToken = cJSON_GetStringValue( cJSON_GetObjectItem( accessRequestObject, "token" ));
                ESP_LOGI( TAG_HTTPCLIENT, "got token %s", signalKServerToken.c_str());
                saveSetting(NVS_KEY_TOKEN, signalKServerToken);
                httpClientData.release();
                triggerWsClientConnect();
            } else {
                // "accessRequest": {"permission": "DENIED"}
                ESP_LOGW( TAG_HTTPCLIENT, "permission %s", permission );
                prepareAccessRequest();
                startPendingTokenTimer();
            }
        }
    } else {
        ESP_LOGW( TAG_HTTPCLIENT, "received %s", buffer );
    }
    cJSON_Delete( result );
    return;

 retry:
    cJSON_Delete( result );
    httpClientData.release();
    pendingTokenState = false;
    startPendingTokenTimer();
}

void EspSigK::prepareAccessRequest() {
    char post_data[96];
    char url[64];

    pendingTokenState = false;
    // see https://signalk.org/specification/1.7.0/doc/access_requests.html
    snprintf( post_data, sizeof( post_data ), R"({"clientId":"%s","description":"%s"})",
              signalKUuid.c_str(), deviceName.c_str());
    snprintf( url, sizeof( url ), "http://%s:%d/signalk/v1/access/requests", signalKServerHost.c_str(), signalKServerPort );
    httpClientData.setPostRequest( url, post_data );
}

void EspSigK::prepareCheckAccessRequest( const char* requestId) {
    char url[96];
    snprintf( url, sizeof( url ), "http://%s:%d/signalk/v1/requests/%s", signalKServerHost.c_str(), signalKServerPort, requestId );

    pendingTokenState = false;
    httpClientData.setGetRequest( url );
}

void EspSigK::sendAccessRequest() {
    std::string requestId;
    loadSetting( NVS_KEY_REQUEST_ID, requestId );
    if ( requestId.empty() ) {
        prepareAccessRequest();
    } else {
        ESP_LOGI(TAG_HTTPCLIENT, "Reusing stored requestId");
        prepareCheckAccessRequest( requestId.c_str() );
    }
    sendAndHandleAccessRequest();
}

void EspSigK::onTokenTimer() {
    if ( pendingTokenState ) {
        sendAndHandleAccessRequest();
    } else {
        sendAccessRequest();
    }
}

void EspSigK::startWsClientConnectTimer() {
    ESP_LOGI( TAG, "Start wsClientConnectTimer" );
    xTimerStart( wsClientConnectTimer, portMAX_DELAY );
}

void EspSigK::startPendingTokenTimer() const {
    ESP_LOGI( TAG, "Start pendingTokenTimer" );
    xTimerStart( pendingTokenTimer, portMAX_DELAY );
}

void EspSigK::clearRequestIdAndRequestNew() {
    clearRequestId();
    prepareAccessRequest();
    startPendingTokenTimer();
}

void EspSigK::clearRequestId() {
    std::string dummy;
    saveSetting( NVS_KEY_REQUEST_ID, dummy );
}
