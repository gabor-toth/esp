#ifndef EspSigK_H
#define EspSigK_H

/**
 * see https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway
 */

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_websocket_client.h"
#include "esp_http_client.h"
#include "freertos/timers.h"
#include "HttpRequest.h"
#include <list>
#include <string>

class EspSigK;

class DeltaValue {
public:
    DeltaValue( const char *path, const char *value );

    DeltaValue( const char *path, const char *value, int type );

    DeltaValue( const char *path, double value );

    DeltaValue( const char *path, bool value );

    // cJSON_Raw/cJSON_Number/cJSON_String
    int type;
    std::string path;
    std::string valueString;
    double valueDouble;
    bool valueBool;
};

class DeltaSet {
public:
    DeltaSet( unsigned char source, unsigned long pgn );

    void addValue( const char *path, const char *value );

    void addValue( const char *path, int value );

    void addValue( const char *path, double value );

    void addValue( const char *path, bool value );

    void addJsonValue( const char *path, const char *value );

    [[nodiscard]] const std::list<DeltaValue> &getDeltas() const {
        return deltas;
    };

    [[nodiscard]] unsigned char getSource() const {
        return source;
    }

    [[nodiscard]] unsigned long getPgn() const {
        return pgn;
    }

    void send( EspSigK &espSigk );

private:
    std::list<DeltaValue> deltas;
    unsigned char source;
    unsigned long pgn;
};

class EspSigK {
private:

public:
    EspSigK();

    ~EspSigK();

    void init();

    void start( const char *deviceName, const char *hostname, httpd_handle_t server, const char *wifi_ssid );

    void stop();

    void setPrintDeltaSerial( bool v );

    void setPrintDebugSerial( bool v );

    void sendDeltaSet( DeltaSet &deltaSet );

private:
    void setupDiscovery();

    void setupHTTP();

    void setupWebSocket();

    bool connectWsClient();

    void stopWsClient();

    void onClientConnected();

    void onClientDisconnected();

    void onClientTextReceived( const char *buf );

    void processFrameHello( cJSON *o );

    bool findMDNSService();

    static esp_err_t htmlSignalKEndpoints( httpd_req_t *r );

    static esp_err_t htmlIndexContents( httpd_req_t *r );

    static esp_err_t htmlDescriptionXml( httpd_req_t *r );

    void onWebSocketClientEvent( esp_event_base_t event_base, int32_t event_id, void *event_data );

    static void webSocketClientEventHandler( void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                             void *event_data );

    _Noreturn static void taskWsClientConnect( void *arg );

    void triggerWsClientConnect();

    static void timerCallback( TimerHandle_t timer );

    bool loadSetting( const char *name, std::string &value );

    void setUuid();

    void sendAccessRequest();

    void prepareAccessRequest();

    void prepareCheckAccessRequest( const char *href );

    void onTokenTimer();

    void sendAndHandleAccessRequest();

    void saveSetting( const char *name, const std::string &value );

    httpd_handle_t http_server;
    std::string hostname;
    std::string ip_address;
    bool printDeltaSerial;
    bool printDebugSerial;

    std::string deviceName;
    std::string signalKServerHost;
    uint16_t signalKServerPort;
    std::string signalKServerToken;
    std::string signalKUuid;
    std::string authHeader;
    esp_websocket_client_handle_t wsClientHandle;
    uint32_t wsClientReconnectInterval;
    static QueueHandle_t event_queue;
    bool wsClientConnected;
    TimerHandle_t wsClientConnectTimer;
    TaskHandle_t wsTask;
    bool pendingTokenState;
    bool logged_no_http_server;
    bool logged_no_ws_connection;
    TimerHandle_t pendingTokenTimer;
    SemaphoreHandle_t semaphore;

    HttpRequest httpClientData;

    void startPendingTokenTimer() const;

    void startWsClientConnectTimer();

    void clearRequestIdAndRequestNew();

    void clearRequestId();

    static void debug_timer_cb( void *arg );

    static void start_free_mem_timer();

    void getIpAddress();
};

extern EspSigK sigK;

#endif
