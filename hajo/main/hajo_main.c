#include "driver/gpio.h"
#include "esp_event.h"
#include "hajo_adc.h"
#include "hajo_can.h"
#include "lib/main_main.h"
#include "lib/nvs_main.h"

static void set_pins(  ) {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
        BIT1 | BIT2 | BIT3
        ;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = true;
    gpio_config( &io_conf );

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        // ADC in BIT0|BIT1|BIT3|
        BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|
        BIT10|BIT11|BIT12|
        // TWAI BIT13 |BIT14 |
        BIT15|BIT16|BIT17|BIT18|BIT19|
        BIT20|BIT21|BIT26|
        BIT33|BIT34|BIT35|BIT36|BIT37|BIT38|BIT39|
        BIT40|BIT41|BIT42|
        // USB UART BIT43|BIT44|
        BIT45
        ;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config( &io_conf );

//    io_conf.intr_type = GPIO_INTR_ANYEDGE;
//    io_conf.mode = GPIO_MODE_INPUT;
//
//    io_conf.pull_down_en = false;
//    io_conf.pull_up_en = true;
//    gpio_config( &io_conf );
}


void app_main() {
    ESP_ERROR_CHECK( esp_event_loop_create_default());
//    main_main();
//    nvs_init();
//    set_pins();
    can_main();
    adc_main();

//    esp_sleep_enable_timer_wakeup(1000000);
//    esp_sleep_enable_ext0_wakeup();
//    esp_sleep_enable_gpio_wakeup();
//    esp_light_sleep_start();
}
