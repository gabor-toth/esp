#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "hajo_adc.h"

// config

typedef struct {
    adc_channel_t channel;
    const char *name;
} channel_config_t;

static const channel_config_t channels[] = {
        {
                .channel = ADC_CHANNEL_0,
                .name = "motor",
        },
        {
                .channel = ADC_CHANNEL_1,
                .name = "munka1",
        },
        {
                .channel = ADC_CHANNEL_2,
                .name = "munka2",
        }
};
static int channel_count = sizeof( channels ) / sizeof( channel_config_t );
static int number_of_samples = 16;  // Multisampling, was originally 64
static int sampling_interval_seconds = 10;

// static variables

static adc_cali_handle_t scheme_handle = NULL;
static adc_oneshot_unit_handle_t unit_handle = NULL;
static QueueHandle_t timer_event_queue = NULL;

static void read_one( const channel_config_t *channel ) {
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
    ESP_LOGI( "adc", "Channel %-8s Raw: %4ld Voltage: %4dmV", channel->name, adc_reading, voltage );
}

static void read_all() {
    for ( int i = 0; i < channel_count; i++ ) {
        read_one( &channels[ i ] );
    }
}

_Noreturn static void timer_task_main( void *arg ) {
    (void) arg;

    for ( ;; ) {
        uint32_t dummy;
        if ( xQueueReceive( timer_event_queue, &dummy, portMAX_DELAY )) {
            read_all();
        }
    }
}

static void timer_callback( TimerHandle_t timer ) {
    (void) timer;

    uint32_t dummy = 0;
    ESP_LOGI( "adc", "tick" );
    xQueueSend( timer_event_queue, &dummy, 0 );
}

static void timer_start() {
    timer_event_queue = xQueueCreate( 10, sizeof( uint32_t ));
    xTaskCreate( timer_task_main, "adc", 2048, NULL, 5, NULL);

    TimerHandle_t timer = xTimerCreate(
            "adc",
            pdMS_TO_TICKS( sampling_interval_seconds * 1000 ),
            1,
            NULL,
            timer_callback );
    xTimerStart( timer, portMAX_DELAY );

    uint32_t dummy = 0;
    xQueueSend( timer_event_queue, &dummy, 0 );
}

void adc_main( void ) {
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
        ESP_ERROR_CHECK( adc_oneshot_config_channel( unit_handle, channels[ i ].channel, &channel_config ));
    }

    timer_start();
}
