#include "wit_main.h"
#include "wit_c_sdk.h"
#include "config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "stdbool.h"

// see https://github.com/WITMOTION/WitStandardProtocol_JY901/blob/main/ESP32/ESP32_sdk/main/main.c

static const char *LOG = "wit";

#define UART_NUM UART_NUM_0

#define BUF_SIZE 1024

#define ACC_UPDATE      0x01
#define GYRO_UPDATE     0x02
#define ANGLE_UPDATE    0x04
#define MAG_UPDATE      0x08
#define READ_UPDATE     0x80

static QueueHandle_t process_event_queue = NULL;
static volatile bool gotMessage;
static volatile bool processing;

static void SensorUartSend( uint8_t *p_data, uint32_t uiSize ) {
    uart_write_bytes( UART_NUM, (const char *) p_data, uiSize );
}

static void UartInit( uint32_t baud_rate ) {
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = (int) baud_rate,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK( uart_driver_install( UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0 ) );
    ESP_ERROR_CHECK( uart_param_config( UART_NUM, &uart_config ) );
    ESP_ERROR_CHECK( uart_set_pin( UART_NUM, GPIO_NUM_GATEWAY_WITMOTION_TX, GPIO_NUM_GATEWAY_WITMOTION_RX, -1, -1 ) );
}

_Noreturn static void receive_task( void *pvParameters ) {
    unsigned char ucTemp[16];

    processing = true;
    while ( 1 ) {
        int read = uart_read_bytes( UART_NUM, ucTemp, sizeof( ucTemp ), portMAX_DELAY );
        for ( int i = 0; i < read; i++ ) {
            WitSerialDataIn( ucTemp[ i ] );
        }
    }
}

_Noreturn static void process_task( void *pvParameters ) {
    (void) pvParameters;

    for ( ;; ) {
        int dataUpdate;

        if ( !xQueueReceive( process_event_queue, &dataUpdate, portMAX_DELAY ) ) {
            continue;
        }
        if ( dataUpdate == 0 ) {
            continue;
        }

        float fAcc[3], fGyro[3], fAngle[3];

        for ( int i = 0; i < 3; i++ ) {
            fAcc[ i ] = (float) sReg[ AX + i ] / 32768.0f * 16.0f;
            fGyro[ i ] = (float) sReg[ GX + i ] / 32768.0f * 2000.0f;
            fAngle[ i ] = (float) sReg[ Roll + i ] / 32768.0f * 180.0f;
        }
        if ( dataUpdate & ACC_UPDATE ) {
            ESP_LOGI( LOG, "acc:%.3f %.3f %.3f", fAcc[ 0 ], fAcc[ 1 ], fAcc[ 2 ] );
        }
        if ( dataUpdate & GYRO_UPDATE ) {
            ESP_LOGI( LOG, "gyro:%.3f %.3f %.3f", fGyro[ 0 ], fGyro[ 1 ], fGyro[ 2 ] );
        }
        if ( dataUpdate & ANGLE_UPDATE ) {
            ESP_LOGI( LOG, "angle:%.3f %.3f %.3f", fAngle[ 0 ], fAngle[ 1 ], fAngle[ 2 ] );
        }
        if ( dataUpdate & MAG_UPDATE ) {
            ESP_LOGI( LOG, "mag:%d %d %d", sReg[ HX ], sReg[ HY ], sReg[ HZ ] );
        }
    }
}

static void DelayMs( uint16_t millis ) {
    vTaskDelay( pdMS_TO_TICKS( millis ) );
}

static void SensorDataUpdate( uint32_t uiReg, uint32_t uiRegNum ) {
    int dataUpdate = 0;

    int i;
    for ( i = 0; i < uiRegNum; i++ ) {
        switch ( uiReg ) {
//            case AX:
//            case AY:
            case AZ:
                dataUpdate |= ACC_UPDATE;
                break;
//            case GX:
//            case GY:
            case GZ:
                dataUpdate |= GYRO_UPDATE;
                break;
//            case HX:
//            case HY:
            case HZ:
                dataUpdate |= MAG_UPDATE;
                break;
//            case Roll:
//            case Pitch:
            case Yaw:
                dataUpdate |= ANGLE_UPDATE;
                break;
            default:
                dataUpdate |= READ_UPDATE;
                break;
        }
        uiReg++;
    }
    gotMessage = true;
    if ( processing ) {
        xQueueSendToBack( process_event_queue, &dataUpdate, 0 );
    }
}

static void scan_task( void *pvParameters ) {
    unsigned char ucTemp;

    ESP_LOGI( LOG, "start scan" );

    int i, iRetry;
    uint32_t c_uiBaud[10] = { 0, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
    bool found = false;

    for ( i = 1; i < 10; i++ ) {
        ESP_LOGI( LOG, "trying baud %lu", c_uiBaud[ i ] );
        uart_set_baudrate( UART_NUM, c_uiBaud[ i ] );
        iRetry = 2;
        do {
            gotMessage = false;
            WitReadReg( AX, 3 );
            DelayMs( 100 );
            if ( gotMessage ) {
                ESP_LOGI( LOG, "found sensor with baud %lu", c_uiBaud[ i ] );
                found = true;
                break;
            }
            iRetry--;
        } while ( iRetry );
    }
    if ( found ) {
//    if ( WitSetUartBaud( WIT_BAUD_115200 ) != WIT_HAL_OK ) {
//    }
//    uart_set_baudrate( UART_NUM, 115200 );
//    if ( WitSetOutputRate( RRATE_10HZ ) != WIT_HAL_OK ) {
//    }
        // if(WitSetBandwidth(BANDWIDTH_256HZ) != WIT_HAL_OK)
        xTaskCreate( receive_task, "wit_receive", 4096, NULL, 5, NULL );
    } else {
        ESP_LOGE( LOG, "can not find sensor" );
    }
    vTaskDelete( NULL );
}

void wit_main() {
    processing = false;
    UartInit( 9600 );

    process_event_queue = xQueueCreate( 10, sizeof( int ) );
    xTaskCreate( process_task, "wit_process", 4096, NULL, 5, NULL );

    WitInit( WIT_PROTOCOL_NORMAL, 0x50 );
    WitSerialWriteRegister( SensorUartSend );
    WitRegisterCallBack( SensorDataUpdate );
    WitDelayMsRegister( DelayMs );

    xTaskCreate( scan_task, "wit_scan", 4096, NULL, 5, NULL );
}

