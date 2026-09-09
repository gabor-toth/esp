#include "sdkconfig.h"

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI)

#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_wifi_types.h"
#include "sntp_main.h"

static const char *LOG_TAG = "sntp";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

static bool is_time_set;

bool sntp_is_time_set() {
    return is_time_set;
}

void local_time_to_buf( char *__restrict _s, size_t _maxsize ) {
    time_t now = 0;
    struct tm timeinfo = { 0 };

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( _s, _maxsize, "%c", &timeinfo );
}

static void print_local_time() {
    char buf[ 64 ];
    local_time_to_buf( buf, sizeof buf );
    ESP_LOGI( LOG_TAG, "The current date/time is: %s", buf );
}

void time_sync_notification_cb( struct timeval *tv ) {
    (void) tv;

    is_time_set = true;
    print_local_time();
}

static void check_if_time_is_set( void ) {
    time_t now;
    struct tm timeinfo;
    time( &now );
    localtime_r( &now, &timeinfo );
    // Is time set? If not, tm_year will be (1970 - 1900).
    if ( timeinfo.tm_year < ( 2016 - 1900 ) ) {
        is_time_set = false;
        ESP_LOGI( LOG_TAG, "Time is not set yet, waiting for NTP." );
    } else {
        is_time_set = true;
        print_local_time();
    }
}

void sntp_init_before_wifi() {
#if LWIP_DHCP_GET_NTP_SRV
    esp_sntp_servermode_dhcp( true ); // accept NTP offers from DHCP server, if any
#endif
}

static void sntp_init_after_wifi( void *event_handler_arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data ) {
    ESP_LOGI( LOG_TAG, "Initializing SNTP" );
    esp_sntp_setoperatingmode( SNTP_OPMODE_POLL );

    setenv( "TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1 ); // Italy rule
    tzset();

    /*
     * If 'NTP over DHCP' is enabled, we set dynamic pool address
     * as a 'secondary' server. It will act as a fallback server in case that address
     * provided via NTP over DHCP is not accessible
     */
#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
    esp_sntp_setservername( 1, CONFIG_SNTP_TIME_SERVER );
#else   /* LWIP_DHCP_GET_NTP_SRV && (SNTP_MAX_SERVERS > 1) */
    // otherwise, use DNS address from a pool
    sntp_setservername( 0, CONFIG_SNTP_TIME_SERVER );
#endif

    // #define CONFIG_LWIP_SNTP_UPDATE_DELAY 3600000

    sntp_set_time_sync_notification_cb( time_sync_notification_cb );
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode( SNTP_SYNC_MODE_SMOOTH );
#endif
    esp_sntp_init();

    ESP_LOGI( LOG_TAG, "List of configured NTP servers:" );

    for ( uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i ) {
        if ( esp_sntp_getservername( i ) ) {
            ESP_LOGI( LOG_TAG, "server %d: %s", i, esp_sntp_getservername( i ) );
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[ INET6_ADDRSTRLEN ];
            ip_addr_t const *ip = esp_sntp_getserver( i );
            if ( ipaddr_ntoa_r( ip, buff, INET6_ADDRSTRLEN ) != NULL )
                ESP_LOGI( LOG_TAG, "server %d: %s", i, buff );
        }
    }
    check_if_time_is_set();
}

static void handler_on_wifi_disconnect( void *event_handler_arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data ) {
    esp_sntp_stop();
}

void sntp_main() {
    sntp_init_before_wifi();

    ESP_ERROR_CHECK(
        esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
            &sntp_init_after_wifi, NULL ) );
    ESP_ERROR_CHECK(
        esp_event_handler_register( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
            &handler_on_wifi_disconnect, NULL ) );
}

#endif
