#include "config.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "n2k_gateway_actisense.h"
#include "N2kMessages.h"
#include "N2kTypes.h"
#include "sdkconfig.h"
#include <ctime>

#if CONFIG_SIGNALK_OVER_ACTISENSE

static const char *TAG = "n2k-gw";

#define UART_NUM UART_NUM_1

class SerialN2kStream : public N2kStream {
    int read() override {
        return 0;
    }

    int peek() override {
        return 0;
    }

    size_t write( const uint8_t *data, size_t size ) override {
        return uart_write_bytes( UART_NUM, data, size );
    }
};

SerialN2kStream serialN2kStream = {};

void correctGpsWeekIfNeeded( uint16_t& since1970 ) {
    // https://en.wikipedia.org/wiki/GPS_week_number_rollover
    // The second rollover occurred on the night of April 6 to 7, 2019
    long lastRollOverDate = 1554588000l / 24 / 60 / 60;
    while ( since1970 < lastRollOverDate ) {
        since1970 += 1024 * 7; // add 1024 weeks
    }
}

const tN2kMsg*  HandleTimeAndDate( const tN2kMsg &N2kMsg , tN2kMsg& newN2kMsg ) {
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    int16_t LocalOffset;

    if (ParseN2kPGN129033( N2kMsg,DaysSince1970, SecondsSinceMidnight,LocalOffset) ) {
        correctGpsWeekIfNeeded(DaysSince1970);
        SetN2kPGN129033(newN2kMsg, DaysSince1970, SecondsSinceMidnight, LocalOffset);
        newN2kMsg.Source = N2kMsg.Source;
        return &newN2kMsg;
    }
    return &N2kMsg;
}

const tN2kMsg* HandleGNSS( const tN2kMsg& N2kMsg, tN2kMsg& newN2kMsg )  {

    unsigned char SID;
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    double Latitude;
    double Longitude;
    double Altitude;
    tN2kGNSStype GNSSType;
    tN2kGNSSmethod GNSSMethod;
    unsigned char nSatellites;
    double HDOP;
    double PDOP;
    double GeoidalSeparation;
    unsigned char nReferenceStations;
    tN2kGNSStype ReferenceStationType;
    uint16_t ReferenceStationID;
    double AgeOfCorrection;

    if ( ParseN2kGNSS( N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSSType,
                       GNSSMethod,
                       nSatellites, HDOP, PDOP, GeoidalSeparation,
                       nReferenceStations, ReferenceStationType, ReferenceStationID, AgeOfCorrection )) {
        correctGpsWeekIfNeeded(DaysSince1970);
        SetN2kPGN129029(newN2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSSType,
                        GNSSMethod,N2kGNSSi_noIntegrityChecking,
                        nSatellites, HDOP, PDOP, GeoidalSeparation,
                        nReferenceStations, ReferenceStationType, ReferenceStationID, AgeOfCorrection);
        newN2kMsg.Source = N2kMsg.Source;
        return &newN2kMsg;
    }
    return &N2kMsg;
}

void sendN2KMessageToSignalKOverActisense( const tN2kMsg &N2kMsg ) {
    tN2kMsg newN2kMsg;
    const tN2kMsg* N2kMsgToSend;
    switch ( N2kMsg.PGN ) {
        case 129029L:
            N2kMsgToSend = HandleGNSS( N2kMsg, newN2kMsg );
            break;
        case 129033L:
            N2kMsgToSend = HandleTimeAndDate( N2kMsg , newN2kMsg );
            break;
        default:
            N2kMsgToSend = &N2kMsg;
            break;
    }
    N2kMsgToSend->SendInActisenseFormat( &serialN2kStream );
}

void setupSignalkOverActisense() {
    ESP_LOGI( TAG, "Starting SignalK over Actisense" );

    uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK( uart_param_config( UART_NUM, &uart_config ) );
    ESP_ERROR_CHECK( uart_set_pin( UART_NUM,
                                   GPIO_NUM_GATEWAY_ACTISENSE_TX, GPIO_NUM_GATEWAY_ACTISENSE_RX,
                                   GPIO_NUM_NC, GPIO_NUM_NC ) );
    const int uart_buffer_size = 1024;
    ESP_ERROR_CHECK( uart_driver_install( UART_NUM, uart_buffer_size, uart_buffer_size,
                                          0, nullptr, 0 ) );
}

#endif
