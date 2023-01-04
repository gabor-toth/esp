//#include "lib/gpio_define.h"
#include "lib/main_main.h"
#include "lib/nvs_main.h"
//#include "lib/rest_main.h"
//#include "lib/sntp_main.h"
//#include "lib/wifi_connect.h"
//#include "gpio_logic.h"
//#include "program.h"
//#include "program_logic.h"
//#include "program_start.h"

#include "esp_event.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "driver/twai.h"

void app_main_old( void ) {
    main_main();

    nvs_init();
//    gpio_logic_init();
//    program_init();
//
//    sntp_init_before_wifi();
//    rest_init_before_wifi();
//
//    wifi_connect();
//
//    rest_init_after_wifi();
//    sntp_init_after_wifi();
//
//    program_logic_init();
//    program_start_init();
}

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/gpio.h"
//#include "driver/adc.h"
//#include "esp_adc_cal.h"

//#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
//#define NO_OF_SAMPLES   64          //Multisampling
//
//static esp_adc_cal_characteristics_t *adc_chars;
//#if CONFIG_IDF_TARGET_ESP32
//static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
//static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
//#elif CONFIG_IDF_TARGET_ESP32S2
//static const adc_channel_t channel = ADC_CHANNEL_6;     // GPIO7 if ADC1, GPIO17 if ADC2
//static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
//#endif
//static const adc_atten_t atten = ADC_ATTEN_DB_0;
//static const adc_unit_t unit = ADC_UNIT_1;


//static void check_efuse(void)
//{
//#if CONFIG_IDF_TARGET_ESP32
//    //Check if TP is burned into eFuse
//    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
//        printf("eFuse Two Point: Supported\n");
//    } else {
//        printf("eFuse Two Point: NOT supported\n");
//    }
//    //Check Vref is burned into eFuse
//    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
//        printf("eFuse Vref: Supported\n");
//    } else {
//        printf("eFuse Vref: NOT supported\n");
//    }
//#elif CONFIG_IDF_TARGET_ESP32S2
//    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
//        printf("eFuse Two Point: Supported\n");
//    } else {
//        printf("Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
//    }
//#else
//#error "This example is configured for ESP32/ESP32S2."
//#endif
//}


//static void print_char_val_type(esp_adc_cal_value_t val_type)
//{
//    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
//        printf("Characterized using Two Point Value\n");
//    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
//        printf("Characterized using eFuse Vref\n");
//    } else {
//        printf("Characterized using Default Vref\n");
//    }
//}


//void app_main_adc(void)
//{
//    //Check if Two Point or Vref are burned into eFuse
//    check_efuse();
//
//    //Configure ADC
//    if (unit == ADC_UNIT_1) {
//        adc1_config_width(width);
//        adc1_config_channel_atten(channel, atten);
//    } else {
//        adc2_config_channel_atten((adc2_channel_t)channel, atten);
//    }
//
//    //Characterize ADC
//    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
//    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
//    print_char_val_type(val_type);
//
//    //Continuously sample ADC1
//    while (1) {
//        uint32_t adc_reading = 0;
//        //Multisampling
//        for (int i = 0; i < NO_OF_SAMPLES; i++) {
//            if (unit == ADC_UNIT_1) {
//                adc_reading += adc1_get_raw((adc1_channel_t)channel);
//            } else {
//                int raw;
//                adc2_get_raw((adc2_channel_t)channel, width, &raw);
//                adc_reading += raw;
//            }
//        }
//        adc_reading /= NO_OF_SAMPLES;
//        //Convert adc_reading to voltage in mV
//        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
//        printf("Raw: %ld\tVoltage: %ldmV\n", adc_reading, voltage);
//        vTaskDelay(pdMS_TO_TICKS(1000));
//    }
//}

//void app_main_twai()
//{
//    //Initialize configuration structures using macro initializers
//    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
//    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
//    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
//
//    //Install TWAI driver
//    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
//        printf("Driver installed\n");
//    } else {
//        printf("Failed to install driver\n");
//        return;
//    }
//
//    //Start TWAI driver
//    if (twai_start() == ESP_OK) {
//        printf("Driver started\n");
//    } else {
//        printf("Failed to start driver\n");
//        return;
//    }
//
//    ESP_ERROR_CHECK( esp_event_loop_create_default());
//}

static QueueHandle_t gpio_evt_queue = NULL;

_Noreturn static void task_main( void *arg ) {
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( gpio_evt_queue, &dummy, portMAX_DELAY )) {
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    xQueueSend( gpio_evt_queue, &dummy, 0 );
}

void program_start_init() {
    gpio_evt_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( task_main, "timer", 2048, NULL, 10, NULL );

    TimerHandle_t timer = xTimerCreate(
            "timer",
            pdMS_TO_TICKS( 30 * 1000 ),
            1,
            NULL,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );
}

void app_main() {
    ESP_ERROR_CHECK( esp_event_loop_create_default());
    program_start_init();
}
