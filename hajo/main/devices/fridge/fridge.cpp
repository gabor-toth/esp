//#include "driver/gpio.h"
//#include "driver/ledc.h"
//#include "driver/pulse_cnt.h"
//#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "fridge.h"

#define DO_ANIMATION 0

static const char *TAG = "fridge";


#if DO_ANIMATION
#define ANIMATION_SECS   10

static double duty_cycle;
static bool ascending;
static int animation_counter;
#endif

static void timer_callback( void *arg ) {
    fridge_fan_timer_handler();
    fridge_temp_timer_handler();

#if DO_ANIMATION
    if ( --animation_counter == 0 ) {
        animation_counter = ANIMATION_SECS;
        duty_cycle += 0.05 * ( ascending ? 1 : -1 );
        if ( duty_cycle >= 1.0 ) {
            ascending = false;
            duty_cycle = 1.0;
        } else if ( duty_cycle <= 0.0 ) {
            ascending = true;
            duty_cycle = 0.0;
        }
    }

    fridge_fan_set_duty_cycle( 0, duty_cycle );
    ESP_LOGI( TAG, "Duty %0.2lf",  duty_cycle );
#endif
}

static void setup_fridge_timer() {
    esp_timer_create_args_t tca = {
            .callback = (esp_timer_cb_t) timer_callback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = nullptr,
            .skip_unhandled_events = false
    };

    esp_timer_handle_t timer = nullptr;
    esp_err_t stat = esp_timer_create( &tca, &timer );
    if (stat != ESP_OK) {
        ESP_LOGE(TAG,"Failed to create timer, err 0x%x\n",stat);
        return;
    }
    esp_timer_start_periodic(timer, 1000000L);
}


void fridge_main() {
    fridge_fan_setup();
    fridge_temp_setup();

#if DO_ANIMATION
    duty_cycle = 0.0;
    ascending = true;
    animation_counter = ANIMATION_SECS;
#endif

    setup_fridge_timer();
}
