#include "driver/i2c.h"
#include "esp_log.h"
#include "gyroscope.h"
#include "mpu6050.h"

static const char *LOG = "yro";

static mpu6050_handle_t gyroscope;

void setup_gyroscope() {
    i2c_port_t i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = GPIO_NUM_4,
            .scl_io_num = GPIO_NUM_6,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
//            .master.clk_speed = 100000, -> either all initializer clauses should be designated or none of them should be
            .clk_flags = 0,
    };
    conf.master.clk_speed = 100000;

    i2c_param_config( i2c_master_port, &conf );
    ESP_ERROR_CHECK( i2c_driver_install( i2c_master_port, conf.mode, 0, 0, 0 ));

    gyroscope = mpu6050_create( i2c_master_port, 0b1101000 );
    mpu6050_wake_up( gyroscope );
    mpu6050_gyro_value_t gyro_value;
    mpu6050_get_gyro( gyroscope, &gyro_value );
    ESP_LOGI( LOG, "gyro x=%lf y=%lf z=%lf", gyro_value.gyro_x, gyro_value.gyro_y, gyro_value.gyro_z );
}

