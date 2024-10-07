#include "config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "math.h"
#include "stdbool.h"
#include "wit_c_sdk.h"
#include "wit_main.h"

// see https://github.com/WITMOTION/WitStandardProtocol_JY901/blob/main/ESP32/ESP32_sdk/main/main.c

static const char *LOG = "wit";

#define UART_NUM UART_NUM_0

// >= UART_HW_FIFO_LEN(uart_num)
#define BUF_SIZE 1024

#define ACC_UPDATE      0x01
#define GYRO_UPDATE     0x02
#define ANGLE_UPDATE    0x04
#define MAG_UPDATE      0x08
#define READ_UPDATE     0x80

#define DEFAULT_FREQUENCY_HZ    (1)
#define SCAN_DELAY_FREQUENCY    (2)
#define SCAN_DELAY_MS           (1000/DEFAULT_FREQUENCY_HZ*SCAN_DELAY_FREQUENCY)

static QueueHandle_t process_event_queue = NULL;
static volatile QueueHandle_t scan_event_queue = NULL;
static volatile bool gotMessage;
static volatile bool processing;
static volatile bool hasAngleData;
static float fAcc[3], fGyro[3], fAngle[3];

static void SensorUartSend( uint8_t *p_data, uint32_t uiSize ) {
    uart_write_bytes( UART_NUM, (const char *) p_data, uiSize );
}

static void uart_init() {
    /* Configure parameters of an UART driver,communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK( uart_param_config( UART_NUM, &uart_config ) );
    ESP_ERROR_CHECK( uart_set_pin( UART_NUM,
                                   GPIO_NUM_GATEWAY_WITMOTION_TX, GPIO_NUM_GATEWAY_WITMOTION_RX,
                                   GPIO_NUM_NC, GPIO_NUM_NC ) );
    ESP_ERROR_CHECK( uart_driver_install( UART_NUM, BUF_SIZE, BUF_SIZE,
                                          0, NULL, 0 ) );
}

static void DelayMs( uint16_t millis ) {
    vTaskDelay( pdMS_TO_TICKS( millis ) );
}

_Noreturn static void receive_task( void *pvParameters ) {
    unsigned char ucTemp[16];

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

        for ( int i = 0; i < 3; i++ ) {
            fAcc[ i ] = (float) sReg[ AX + i ] / 32768.0f * 16.0f;
            fGyro[ i ] = (float) sReg[ GX + i ] / 32768.0f * 2000.0f;
            fAngle[ i ] = (float) sReg[ Roll + i ] / 32768.0f * 180.0f;
        }
        if ( dataUpdate & ACC_UPDATE ) {
            ESP_LOGD( LOG, "acc:%.3f %.3f %.3f", fAcc[ 0 ], fAcc[ 1 ], fAcc[ 2 ] );
        }
        if ( dataUpdate & GYRO_UPDATE ) {
            ESP_LOGD( LOG, "gyro:%.3f %.3f %.3f", fGyro[ 0 ], fGyro[ 1 ], fGyro[ 2 ] );
        }
        if ( dataUpdate & ANGLE_UPDATE ) {
            ESP_LOGD( LOG, "angle:%.3f %.3f %.3f", fAngle[ 0 ], fAngle[ 1 ], fAngle[ 2 ] );
            hasAngleData = true;
        }
        if ( dataUpdate & MAG_UPDATE ) {
            ESP_LOGD( LOG, "mag:%d %d %d", sReg[ HX ], sReg[ HY ], sReg[ HZ ] );
        }
    }
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
    } else if ( scan_event_queue != NULL ) {
        int dummy = 0;
        xQueueSendToBack( scan_event_queue, &dummy, 0 );
    }
}

static void startProcessing() {
    processing = true;
}

static void scan_timer( TimerHandle_t xTimer ) {
    int dummy = 0;
    xQueueSendToBack( scan_event_queue, &dummy, 0 );
}

static bool findSensor() {
    scan_event_queue = xQueueCreate( 10, sizeof( int ) );
    TimerHandle_t timer = xTimerCreate( NULL, pdMS_TO_TICKS( SCAN_DELAY_MS ), false, NULL, scan_timer );

    bool found = false;
    int i, iRetry;
    uint32_t c_uiBaud[] = {
            //4800,
            115200,
            9600,
            19200, 38400, 57600, 230400, 460800, 921600 };
    for ( i = 0; i < sizeof( c_uiBaud ) / sizeof( c_uiBaud[ 0 ] ) && !found; i++ ) {
        uart_set_baudrate( UART_NUM, c_uiBaud[ i ] );
        uint32_t effective_baud_rate = 0;
        uart_get_baudrate( UART_NUM, &effective_baud_rate );
        ESP_LOGI( LOG, "trying baud %lu (effective %lu)", c_uiBaud[ i ], effective_baud_rate );
        gotMessage = false;
        iRetry = 2;
        do {
            WitReadReg( AX, 3 );
            xTimerStart( timer, portMAX_DELAY );
            int dummy;
            xQueueReceive( scan_event_queue, &dummy, portMAX_DELAY );
            if ( gotMessage ) {
                ESP_LOGI( LOG, "found sensor with baud %lu", c_uiBaud[ i ] );
                found = true;
                break;
            }
            //iRetry--;
        } while ( iRetry );
    }

    xTimerDelete( timer, portMAX_DELAY );

    QueueHandle_t queue = scan_event_queue;
    scan_event_queue = NULL;
    vQueueDelete( queue );

    return found;
}

static void configureSensor() {
    if ( WitSetUartBaud( WIT_BAUD_115200 ) != WIT_HAL_OK ) {
        ESP_LOGE( LOG, "Failed to set wit baud rate to 115200 (%d)", WIT_BAUD_115200 );
    } else {
        int baudrate = 115200;
        uart_set_baudrate( UART_NUM, baudrate );
        ESP_LOGI( LOG, "Set baud rate to %d", baudrate );
    }
    int outputRate = 10;
    if ( WitSetOutputRate( RRATE_10HZ ) != WIT_HAL_OK ) {
        ESP_LOGE( LOG, "Failed to set output rate to %dHz (%d)", outputRate, RRATE_10HZ );
    } else {
        ESP_LOGI( LOG, "Set output rate to %dHz", outputRate );
    }
    // if(WitSetBandwidth(BANDWIDTH_256HZ) != WIT_HAL_OK)
}

static void scan_task( void *pvParameters ) {
    ESP_LOGI( LOG, "start scan" );

    bool found = findSensor();
    if ( found ) {
        configureSensor();
        startProcessing();
    } else {
        ESP_LOGE( LOG, "can not find sensor" );
    }
    vTaskDelete( NULL );
}

static void startScanning() {
    xTaskCreate( scan_task, "wit_scan", 4096, NULL, 5, NULL );
}

bool wit_sensor_is_available() {
    return hasAngleData;
}

void wit_sensor_get_pitch_and_roll( float *pitch, float *roll ) {
    if ( hasAngleData ) {
        *pitch = fAngle[ 0 ];
        *roll = fAngle[ 1 ];
    } else {
        *pitch = *roll = 0;
    }
}

void wit_sensor_get_heading( float *heading ) {
    if ( hasAngleData ) {
        *heading = (float) fmod( 360.0 + 90.0 - fAngle[ 2 ], 360.0 );
    } else {
        *heading = 0;
    }
}

void wit_sensor_start() {
    hasAngleData = processing = false;
    uart_init();

    process_event_queue = xQueueCreate( 10, sizeof( int ) );
    xTaskCreate( process_task, "wit_process", 4096, NULL, 5, NULL );
    xTaskCreate( receive_task, "wit_receive", 4096, NULL, 5, NULL );

    WitInit( WIT_PROTOCOL_NORMAL, 0x50 );
    WitSerialWriteRegister( SensorUartSend );
    WitRegisterCallBack( SensorDataUpdate );
    WitDelayMsRegister( DelayMs );

    startScanning();
}
