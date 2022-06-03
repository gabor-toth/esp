#include "lib/gpio_define.h"
#include "test_util.h"
#include "lib/rest_main.h"
#include "lib/nvs_main.h"
#include "lib/sntp_main.h"

void app_main( void ) {
    sntp_init_before_wifi();
    nvs_init();
    gpio_init();
    rest_init();
    sntp_init_after_wifi();
//    test_init();
}
