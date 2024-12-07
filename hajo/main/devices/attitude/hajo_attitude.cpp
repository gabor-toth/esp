#include "hajo_attitude.h"
#include "config.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "mpu6050.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_util.h"
#include <cmath>

#define RAD_TO_DEG                  57.27272727f /*!< Radians to degrees */

static const char *TAG = "hajo_atti";

#define LOG_LEVEL   ESP_LOG_DEBUG
#define LOG(format, ... ) ESP_LOG_LEVEL_LOCAL(LOG_LEVEL, TAG, format, ##__VA_ARGS__)
#define DO_LOG_READINGS   0
#define CONFIG_RETRIES  5

static mpu6050_handle_t gyroscope;
static int myDeviceIndex;

static complimentary_angle_t correction = {
        .roll = 0.0,
        .pitch = 0.0
};

void setup_gyroscope() {
    i2c_port_t i2c_master_port = I2C_NUM_0;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = BIT(GPIO_ATTITUDE_GND) | BIT(GPIO_ATTITUDE_3V3);
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config( &io_conf );

    gpio_set_level(GPIO_ATTITUDE_GND, 0);
    gpio_set_level(GPIO_ATTITUDE_3V3, 1);

    vTaskDelay(pdMS_TO_TICKS(100));

    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = GPIO_ATTITUDE_I2C_SDA,
            .scl_io_num = GPIO_ATTITUDE_I2C_SDC,
            .sda_pullup_en = true,
            .scl_pullup_en = true,
            .master {
                    .clk_speed = 100000, //-> either all initializer clauses should be designated or none of them should be
            },
            .clk_flags = 0,
    };
    conf.master.clk_speed = 100000;

    i2c_param_config( i2c_master_port, &conf );
    ESP_ERROR_CHECK( i2c_driver_install( i2c_master_port, conf.mode, 0, 0, 0 ));

    gyroscope = mpu6050_create( i2c_master_port, 0b1101000 );
    for( int retry = 1; retry<= CONFIG_RETRIES; retry++) {
        esp_err_t  result = mpu6050_config( gyroscope, ACCE_FS_2G, GYRO_FS_250DPS );
        if ( result == ESP_OK ) {
            break;
        }
        if ( retry == CONFIG_RETRIES) {
            ESP_ERROR_CHECK( result );
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_ERROR_CHECK(mpu6050_wake_up( gyroscope ) );
}

static bool send_attitude( int index, tN2kMsg &msg, int &deviceIndex ) {
//    mpu6050_gyro_value_t gyro_value;
//    mpu6050_get_gyro( gyroscope, &gyro_value );
    mpu6050_acce_value_t acce_value;
    ESP_ERROR_CHECK(mpu6050_get_acce( gyroscope, &acce_value ));
    complimentary_angle_t angle;
//    mpu6050_complimentory_filter( gyroscope, &acce_value, &gyro_value, &angle );
    // see https://howthingsfly.si.edu/flight-dynamics/roll-pitch-and-yaw
    // roll = left - right (around x axis)
    // pitch = front - back (around y axis)
    // yaw = turning  (around z axis)

    // board mounted horizontally
    // angle.roll = atan2(acce_value.acce_y, acce_value.acce_z);
    // angle.pitch = atan2(acce_value.acce_x, acce_value.acce_z) ;
    // board mounted vertically, facing forward
    angle.roll = -atan2( acce_value.acce_y, acce_value.acce_x ) + correction.roll;
    angle.pitch = atan2( acce_value.acce_z, acce_value.acce_x ) + correction.pitch;

#if DO_LOG_READINGS
    ESP_LOGI( TAG, "angle roll=%lf pitch=%lf  acce x=%lf y=%lf z=%lf",
              angle.roll* RAD_TO_DEG, angle.pitch * RAD_TO_DEG,
              acce_value.acce_x, acce_value.acce_y, acce_value.acce_z
              );
//    ESP_LOGI( TAG, "angle roll=%lf pitch=%lf  gyro x=%lf y=%lf z=%lf   acce x=%lf y=%lf z=%lf",
//              angle.roll, angle.pitch,
//              gyro_value.gyro_x, gyro_value.gyro_y, gyro_value.gyro_z ,
//              acce_value.acce_x, acce_value.acce_y, acce_value.acce_z
//              );
#endif
    // values are in rad, see https://signalk.org/specification/1.5.0/doc/vesselsBranch.html#vesselsregexpnavigationattitude
    LOG("SetN2kAttitude");
    deviceIndex = myDeviceIndex;
    SetN2kAttitude( msg, index, 0.0, angle.pitch, angle.roll );
    return true;
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_ATTITUDE,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            100,                        // Manufacturer's product code
            "Attitude",                 // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=Attitude
                                   60,        // Device class=Navigation
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool n2k_send_attitude( int index, tN2kMsg &message, int &deviceIndex ) {
    switch ( index ) {
        case 0:
            return send_attitude( index, message, deviceIndex );
        default:
            return false;
    }
}

void hajo_attitude_main( int iDev ) {
    myDeviceIndex = iDev;
    LOG("hajo_attitude_main");

    // TODO read correction from nvs

    setup_gyroscope();

    setup_n2k_device( iDev );
    nk2_register_sender( n2k_send_attitude, "attitude", N2K_PGN_ATTITUDE_INTERVAL_MS, 320, true );
}
