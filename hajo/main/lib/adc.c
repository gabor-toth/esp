#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "adc.h"
#include "string.h"

static const char *LOG = "adc";

#define MAX_CHANNELS 8

typedef struct {
    adc_channel_t channel;
    const char *name;
    uint8_t instance_id;
    uint8_t instance_type;
    adc_value_converter converter;
    int raw_value;
    uint32_t converted_value;
} adc_channel_data_t;

static adc_channel_data_t channels[MAX_CHANNELS];
static int channel_count = 0;
static int number_of_samples = 16;  // Multisampling, was originally 64
static int sampling_interval_seconds = 1;

// static variables

static adc_cali_handle_t scheme_handle = NULL;
static adc_oneshot_unit_handle_t unit_handle = NULL;
static QueueHandle_t timer_event_queue = NULL;

static void read_one( adc_channel_data_t *channel ) {
    uint32_t adc_reading = 0;
    for ( int i = 0; i < number_of_samples; i++ ) {
        int raw = 0;
        // TODO ESP_ERROR_CHECK == ESP_ERR_TIMEOUT
        adc_oneshot_read( unit_handle, channel->channel, &raw );
        adc_reading += raw;
    }
    adc_reading /= number_of_samples;
    int voltage;
    ESP_ERROR_CHECK( adc_cali_raw_to_voltage( scheme_handle, adc_reading, &voltage ));
    channel->raw_value = voltage;
    uint32_t correction = 0;
    if ( channel->converter ) {
        channel->converter( channel->raw_value, &channel->converted_value, &correction );
    }
    ESP_LOGI( LOG, "Channel %d %-10s Raw: %4ld Voltage: %4dmV Display: %5ld (%ld)",
              channel->channel,
              channel->name,
              adc_reading,
              voltage,
              channel->converted_value,
              correction );
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
        if ( xQueueReceive( timer_event_queue, &dummy, portMAX_DELAY )) {
            adc_read_all();
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    (void) timer;

    uint32_t dummy = 0;
    ESP_LOGI( LOG, "tick" );
    xQueueSend( timer_event_queue, &dummy, 0 );
}

static void timer_start() {
    timer_event_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( timer_task_main, LOG, 2048, NULL, 5, NULL );

    TimerHandle_t timer = xTimerCreate(
            LOG,
            pdMS_TO_TICKS( sampling_interval_seconds * 1000 ),
            1,
            NULL,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );

    uint32_t dummy = 0;
    xQueueSend( timer_event_queue, &dummy, 0 );
}

int adc_number_of_channels() {
    return channel_count;
}

extern esp_err_t adc_get_channel_value( int index, adc_channel_value_t *channel_value ) {
    if ( index < 0 || index > channel_count ) {
        return ESP_FAIL;
    }
    adc_channel_data_t *channel = &channels[ index ];
    channel_value->instance = channel->instance_id;
    channel_value->type = channel->instance_type;
    channel_value->value = channel->converter != NULL
                           ? channel->converted_value // channel->converter( channel->raw_value )
                           : channel->raw_value;
    return ESP_OK;
}

esp_err_t adc_add_channel( uint8_t adc_channel, const char *name, uint8_t instance, uint8_t type,
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
    adc_channel_data_t *channel = &channels[ channel_count++ ];
    memset( channel, 0, sizeof( *channel ));
    channel->channel = (adc_channel_t) adc_channel;
    channel->converter = converter;
    channel->instance_id = instance;
    channel->instance_type = type;
    channel->name = strdup( name );
    return ESP_OK;
}

void adc_main( void ) {
    ESP_LOGI( LOG, "adc start" );

    adc_cali_line_fitting_config_t cali_config = {
            .atten = ADC_ATTEN_DB_0,
            .bitwidth = ADC_BITWIDTH_13,
            .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK( adc_cali_create_scheme_line_fitting( &cali_config, &scheme_handle ));

    adc_oneshot_unit_init_cfg_t unit_config = {
            .ulp_mode = ADC_ULP_MODE_DISABLE,
            .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK( adc_oneshot_new_unit( &unit_config, &unit_handle ));
    const adc_oneshot_chan_cfg_t channel_config = {
            .atten = ADC_ATTEN_DB_0,
            .bitwidth = ADC_BITWIDTH_13,
    };

    for ( int i = 0; i < channel_count; i++ ) {
        ESP_LOGI( LOG, "add adc channel %d", channels[ i ].channel );
        ESP_ERROR_CHECK( adc_oneshot_config_channel( unit_handle, channels[ i ].channel, &channel_config ));
    }

    timer_start();

    ESP_LOGI( LOG, "adc started" );
}
