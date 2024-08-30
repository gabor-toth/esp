#include "config.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "n2k_gateway_actisense.h"
#include "sdkconfig.h"

#if CONFIG_SIGNALK_OVER_ACTISENSE

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

void sendN2KMessageToSignalKOverActisense( const tN2kMsg &N2kMsg ) {
    N2kMsg.SendInActisenseFormat( &serialN2kStream );
}

void setupSignalkOverActisense() {
    uart_config_t uart_config = {
//            .baud_rate = 921600,
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK( uart_param_config( UART_NUM, &uart_config ) );
    ESP_ERROR_CHECK(
            uart_set_pin( UART_NUM, GPIO_NUM_GATEWAY_UART_TX, GPIO_NUM_GATEWAY_UART_RX, GPIO_NUM_NC, GPIO_NUM_NC ) );
    const int uart_buffer_size = ( 1024 * 2 );
//    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK( uart_driver_install( UART_NUM, uart_buffer_size,
                                          uart_buffer_size, 0, NULL, 0 ) );
//                                          uart_buffer_size, 10, &uart_queue, 0 ) );
}

#endif
