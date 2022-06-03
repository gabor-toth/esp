#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "sntp_main.h"

static const char *LOG_TAG = "sntp";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

static bool is_time_set;

bool sntp_is_time_set() {
    return is_time_set;
}

static void print_local_time() {
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char strftime_buf[64];

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( strftime_buf, sizeof( strftime_buf ), "%c", &timeinfo );
    ESP_LOGI( LOG_TAG, "The current date/time is: %s", strftime_buf );
}

void time_sync_notification_cb( struct timeval *tv ) {
    is_time_set = true;
    print_local_time();
}

static void check_if_time_is_set( void ) {
    time_t now;
    struct tm timeinfo;
    time( &now );
    localtime_r( &now, &timeinfo );
    // Is time set? If not, tm_year will be (1970 - 1900).
    if ( timeinfo.tm_year < ( 2016 - 1900 )) {
        is_time_set = false;
        ESP_LOGI( LOG_TAG, "Time is not set yet, waiting for NTP." );
    } else {
        is_time_set = true;
        print_local_time();
    }
}

void sntp_init_before_wifi() {
#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp( 1 );      // accept NTP offers from DHCP server, if any
#endif
}

void sntp_init_after_wifi() {
    ESP_LOGI( LOG_TAG, "Initializing SNTP" );
    sntp_setoperatingmode( SNTP_OPMODE_POLL );

    setenv( "TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1 ); // Italy rule
    tzset();

/*
 * If 'NTP over DHCP' is enabled, we set dynamic pool address
 * as a 'secondary' server. It will act as a fallback server in case that address
 * provided via NTP over DHCP is not accessible
 */
#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
    sntp_setservername( 1, "pool.ntp.org" );
#else   /* LWIP_DHCP_GET_NTP_SRV && (SNTP_MAX_SERVERS > 1) */
    // otherwise, use DNS address from a pool
    sntp_setservername( 0, CONFIG_SNTP_TIME_SERVER );
#endif

    sntp_set_time_sync_notification_cb( time_sync_notification_cb );
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();

    ESP_LOGI( LOG_TAG, "List of configured NTP servers:" );

    for ( uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i ) {
        if ( sntp_getservername( i )) {
            ESP_LOGI( LOG_TAG, "server %d: %s", i, sntp_getservername( i ));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = sntp_getserver( i );
            if ( ipaddr_ntoa_r( ip, buff, INET6_ADDRSTRLEN ) != NULL)
                ESP_LOGI( LOG_TAG, "server %d: %s", i, buff );
        }
    }
    check_if_time_is_set();
}
