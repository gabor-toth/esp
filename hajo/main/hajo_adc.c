#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "hajo_adc.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate

static esp_adc_cal_characteristics_t *adc_chars;
#if CONFIG_IDF_TARGET_ESP32
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#elif CONFIG_IDF_TARGET_ESP32S2
static const adc1_channel_t adc_channel1 = ADC1_CHANNEL_0;     // GPIO1
static const adc1_channel_t adc_channel2 = ADC1_CHANNEL_1;     // GPIO2
static const adc1_channel_t adc_channel3 = ADC1_CHANNEL_2;     // GPIO3
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
#endif
static const adc_atten_t adc_attenuation = ADC_ATTEN_DB_0;
static const adc_unit_t adc_unit = ADC_UNIT_1;
static int adc_number_of_samples = 64;  // Multisampling, was originally 64


static void check_efuse(void)
{
#if CONFIG_IDF_TARGET_ESP32
    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
#elif CONFIG_IDF_TARGET_ESP32S2
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
    }
#else
#error "This example is configured for ESP32/ESP32S2."
#endif
}


static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

void adc_read(const char* name, adc1_channel_t channel) {
//    adc2_vref_to_gpio();
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < adc_number_of_samples; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)channel);
    }
    adc_reading /= adc_number_of_samples;
    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGI("adc","Channel %8s Raw: %4ld\tVoltage: %4ldmV", name, adc_reading, voltage);

    //  esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw)
}

static QueueHandle_t adc_timer_event_queue = NULL;

_Noreturn static void adc_timer_event( void *arg ) {
    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( adc_timer_event_queue, &dummy, portMAX_DELAY )) {
            adc_read("motor",adc_channel1);
            adc_read("munka1",adc_channel2);
            adc_read("munka2",adc_channel3);
        }
    }
}

static void adc_timer_callback( TimerHandle_t timer ) {
    uint32_t dummy = 0;
    ESP_LOGI("adc","tick");
    xQueueSend( adc_timer_event_queue, &dummy, 0 );
}

void adc_timer_start() {
    adc_timer_event_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( adc_timer_event, "adc", 2048, NULL, 5, NULL);

    TimerHandle_t timer = xTimerCreate(
            "adc",
            pdMS_TO_TICKS( 10*1000 ),
            1,
            NULL,
            adc_timer_callback );
    xTimerStart( timer, portMAX_DELAY );

    uint32_t dummy = 0;
    xQueueSend( adc_timer_event_queue, &dummy, 0 );
}

void adc_main(void)
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    adc1_config_width(width);
    adc1_config_channel_atten( adc_channel1, adc_attenuation);
    adc1_config_channel_atten( adc_channel2, adc_attenuation);
    adc1_config_channel_atten( adc_channel3, adc_attenuation);

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize( adc_unit, adc_attenuation, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
    adc_timer_start();
}
