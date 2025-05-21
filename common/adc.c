#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "adc.h"
#include "string.h"

static const char *LOG = "adc";

// for one shot

#define MAX_CHANNELS 8

typedef struct {
    adc_channel_t channel;
    const char *name;
    adc_value_converter converter;
    int raw_value;
    int converted_value;
    void *user_data;
} adc_channel_internal_t;

static adc_channel_internal_t channels[MAX_CHANNELS];
static int channel_count = 0;
static int number_of_samples = 64;  // Multisampling, was originally 64
static int sampling_interval_seconds = 1;
static bool is_timer_started = false;
static adc_cali_handle_t scheme_handle = NULL;
static adc_oneshot_unit_handle_t oneshot_unit_handle = NULL;
static QueueHandle_t timer_event_queue = NULL;

// for continuous

#define CONTINUOUS_BITWIDTH            ADC_BITWIDTH_12
#define CONTINUOUS_INTERVAL_MS         (100)
#define CONTINUOUS_SAMPLES_PER_FRAME   (100)
#define CONTINUOUS_ROTATE_BY           (3)
//#define CONTINUOUS_INTERVAL_MS         (50)
//#define CONTINUOUS_SAMPLES_PER_FRAME   (100)
#define CONTINUOUS_SAMPLE_FREQUENCY    (CONTINUOUS_SAMPLES_PER_FRAME*1000/CONTINUOUS_INTERVAL_MS)

static adc_continuous_handle_t continuous_handle = NULL;
static TaskHandle_t continuous_task;
static adc_continuous_data_callback_t continuous_data_callback;

// code

static void read_one( adc_channel_internal_t *channel ) {
    int sum_reading = 0;
    int count_reading = 0;
    for ( int i = 0; i < number_of_samples; i++ ) {
        int raw = 0;
        // TODO ESP_ERROR_CHECK == ESP_ERR_TIMEOUT
        if ( ESP_OK == adc_oneshot_read( oneshot_unit_handle, channel->channel, &raw ) ) {
            sum_reading += raw;
            count_reading++;
        }
    }
    int average_raw = sum_reading / count_reading;
    int voltage;
    ESP_ERROR_CHECK( adc_cali_raw_to_voltage( scheme_handle, average_raw, &voltage ) );
    channel->raw_value = voltage;
    int correction = 0;
    if ( channel->converter ) {
        channel->converter( voltage, &channel->converted_value, &correction );
    } else {
        channel->converted_value = voltage;
    }
    ESP_LOGI( LOG, "Channel %d %-10s Raw: %4d Voltage: %4dmV Display: %5d (corr %d, samples %d)",
              channel->channel,
              channel->name,
              average_raw,
              channel->raw_value,
              channel->converted_value,
              correction,
              count_reading );
}

void adc_read_all() {
    for ( int i = 0; i < channel_count; i++ ) {
        read_one( &channels[ i ] );
    }
}

_Noreturn static void timer_task_main( void *arg ) {
    (void) arg;

    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( timer_event_queue, &dummy, portMAX_DELAY ) ) {
            adc_read_all();
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    (void) timer;

    uint32_t dummy = 0;
    ESP_LOGD( LOG, "tick" );
    xQueueSendToBack( timer_event_queue, &dummy, 0 );
}

static void timer_start() {
    is_timer_started = true;
    timer_event_queue = xQueueCreate( 10, sizeof( uint32_t ) );
    xTaskCreate( timer_task_main, LOG, 2048, NULL, 5, NULL );

    TimerHandle_t timer = xTimerCreate(
            LOG,
            pdMS_TO_TICKS( sampling_interval_seconds * 1000 ),
            1,
            NULL,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );

    uint32_t dummy = 0;
    xQueueSendToBack( timer_event_queue, &dummy, 0 );
}

int adc_number_of_channels() {
    return channel_count;
}

esp_err_t adc_get_channel_value( int index, adc_channel_value_t *channel_value ) {
    if ( index < 0 || index > channel_count ) {
        return ESP_FAIL;
    }
    adc_channel_internal_t *channel = &channels[ index ];
    if ( !is_timer_started ) {
        read_one( channel );
    }
    channel_value->channel = channel->channel;
    channel_value->name = channel->name;
    channel_value->user_data = channel->user_data;
    channel_value->raw_value = channel->raw_value;
    channel_value->display_value = channel->converter != NULL
                                   ? channel->converted_value
                                   : channel->raw_value;
    return ESP_OK;
}

esp_err_t adc_get_channel_data( int index, adc_channel_data_t *channel_value ) {
    if ( index < 0 || index > channel_count ) {
        return ESP_FAIL;
    }
    adc_channel_internal_t *channel = &channels[ index ];
    channel_value->channel = channel->channel;
    channel_value->name = channel->name;
    channel_value->user_data = channel->user_data;
    return ESP_OK;
}

void *adc_get_channel_user_data( int index ) {
    if ( index < 0 || index > channel_count ) {
        ESP_LOGE( LOG, "Bad adc channel index %d", index );
        return NULL;
    }
    return channels[ index ].user_data;
}

esp_err_t adc_add_channel( uint8_t adc_channel, const char *name, void *user_data, size_t user_data_bytes,
                           adc_value_converter converter ) {
    if ( channel_count == MAX_CHANNELS ) {
//        ESP_RETURN_ON_FALSE(handle && config, ESP_ERR_INVALID_ARG, TAG, "invalid argument: null pointer");
        ESP_ERROR_CHECK( ESP_ERR_INVALID_SIZE );
        return ESP_ERR_INVALID_SIZE;
    }
    for ( int i = 0; i < channel_count; i++ ) {
        if ( channels[ i ].channel == adc_channel ) {
            ESP_LOGE( LOG, "Channel %d is already used at position %d", adc_channel, i );
            ESP_ERROR_CHECK( ESP_ERR_INVALID_ARG );
        }
    }
    adc_channel_internal_t *channel = &channels[ channel_count++ ];
    memset( channel, 0, sizeof( *channel ) );
    channel->channel = (adc_channel_t) adc_channel;
    channel->converter = converter;
    if ( user_data == NULL || user_data_bytes == 0 ) {
        channel->user_data = NULL;
    } else {
        channel->user_data = malloc( user_data_bytes );
        memcpy( channel->user_data, user_data, user_data_bytes );
    }
    channel->name = strdup( name );
    return ESP_OK;
}

void adc_main_oneshot( bool start_timer ) {
    ESP_LOGI( LOG, "adc start oneshot" );

    adc_cali_line_fitting_config_t cali_config = {
            .atten = ADC_ATTEN_DB_0,
            .bitwidth = ADC_BITWIDTH_13,
            .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK( adc_cali_create_scheme_line_fitting( &cali_config, &scheme_handle ) );

    adc_oneshot_unit_init_cfg_t unit_config = {
            .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
            .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK( adc_oneshot_new_unit( &unit_config, &oneshot_unit_handle ) );
    const adc_oneshot_chan_cfg_t channel_config = {
            .atten = cali_config.atten,
            .bitwidth = cali_config.bitwidth,
    };

    for ( int i = 0; i < channel_count; i++ ) {
        ESP_LOGI( LOG, "add adc channel %d", channels[ i ].channel );
        ESP_ERROR_CHECK( adc_oneshot_config_channel( oneshot_unit_handle, channels[ i ].channel, &channel_config ) );
    }

    if ( start_timer ) {
        timer_start();
        ESP_LOGI( LOG, "adc started with timer" );
    } else {
        ESP_LOGI( LOG, "adc started without timer" );
    }
}

static bool
continuous_callback( adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data ) {
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR( continuous_task, &mustYield );
    return ( mustYield == pdTRUE );
}

_Noreturn static void continuous_task_main( void *arg ) {
    (void) arg;

    size_t buffer_size = CONTINUOUS_SAMPLES_PER_FRAME * SOC_ADC_DIGI_DATA_BYTES_PER_CONV;
    adc_digi_output_data_t *result = malloc( buffer_size );

    for ( ;; ) {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        while ( 1 ) {
            uint32_t ret_num = 0;
            esp_err_t ret = adc_continuous_read( continuous_handle, (uint8_t *) result, buffer_size, &ret_num, 0 );
            if ( ret == ESP_ERR_TIMEOUT ) {
                break;
            }
            if ( ret != ESP_OK ) {
                break;
            }
            adc_digi_output_data_t *p = result;
            uint32_t sum = 0;
            uint32_t count = 0;
            ret_num /= SOC_ADC_DIGI_DATA_BYTES_PER_CONV;
            for ( int i = 0; i < ret_num; i++, p++ ) {
                uint16_t channel = p->type1.channel;
                /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                if ( channel >= SOC_ADC_CHANNEL_NUM( EXAMPLE_ADC_UNIT ) ) {
                    continue;
                }
                sum += p->type1.data;
                count++;
            }
            uint16_t average = sum / count;
            if ( continuous_data_callback ) {
                average &= ~( ( 1 << CONTINUOUS_ROTATE_BY ) - 1 );
                average <<= SOC_ADC_RTC_MAX_BITWIDTH - SOC_ADC_DIGI_MAX_BITWIDTH; // normalize to RTC bits
                int voltage = 0;
                ESP_ERROR_CHECK( adc_cali_raw_to_voltage( scheme_handle, average, &voltage ) );
                continuous_data_callback( average, voltage );
                vTaskDelay( 0 );

            } else {
                average >>= CONTINUOUS_ROTATE_BY;
                ESP_LOGI( LOG, "average %d (bits %d)", average, CONTINUOUS_BITWIDTH - CONTINUOUS_ROTATE_BY );
            }
        }
    }
}

void adc_main_continuous( adc_continuous_data_callback_t callback ) {
    ESP_LOGI( LOG, "adc start continuous" );

    continuous_data_callback = callback;

    xTaskCreate( continuous_task_main, LOG, 3072, NULL, 5, &continuous_task );

    adc_cali_line_fitting_config_t cali_config = {
            .atten = ADC_ATTEN_DB_0,
            .bitwidth = CONTINUOUS_BITWIDTH,
            .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK( adc_cali_create_scheme_line_fitting( &cali_config, &scheme_handle ) );

    adc_continuous_handle_cfg_t handler_config = {
            .max_store_buf_size = CONTINUOUS_SAMPLES_PER_FRAME * SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 4,
            .conv_frame_size = CONTINUOUS_SAMPLES_PER_FRAME * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
            .flags.flush_pool = true
    };
    ESP_ERROR_CHECK( adc_continuous_new_handle( &handler_config, &continuous_handle ) );

    adc_continuous_config_t config = {
            .pattern_num = channel_count,
            .adc_pattern = calloc( channel_count, sizeof( adc_digi_pattern_config_t ) ),
            .sample_freq_hz = CONTINUOUS_SAMPLE_FREQUENCY,
            .conv_mode = ADC_CONV_SINGLE_UNIT_1,
            .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1
    };
    ESP_LOGI( LOG, "adc continuous config: unit=%d, bits=%d, attenuation=%d, frame_size=%ld bytes, sample_freq=%ld Hz",
              cali_config.unit_id,
              cali_config.bitwidth,
              cali_config.atten,
              handler_config.conv_frame_size,
              config.sample_freq_hz );
    for ( int i = 0; i < channel_count; i++ ) {
        ESP_LOGI( LOG, "add adc channel %d", channels[ i ].channel );
        config.adc_pattern[ i ] = (adc_digi_pattern_config_t) {
                .atten = cali_config.atten,
                .channel = channels[ i ].channel,
                .unit = cali_config.unit_id,
                .bit_width = cali_config.bitwidth
        };
    }
    ESP_ERROR_CHECK( adc_continuous_config( continuous_handle, &config ) );
    adc_continuous_evt_cbs_t callbacks = {
            .on_conv_done = continuous_callback,
            .on_pool_ovf = NULL
    };
    ESP_ERROR_CHECK( adc_continuous_register_event_callbacks( continuous_handle, &callbacks, NULL ) );

    ESP_ERROR_CHECK( adc_continuous_start( continuous_handle ) );
    ESP_LOGI( LOG, "adc started continuous" );
}
