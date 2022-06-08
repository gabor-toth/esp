#include "lib/gpio_define.h"
#include "lib/nvs_main.h"
#include "lib/rest_main.h"
#include "lib/sntp_main.h"
#include "gpio_logic.h"
#include "program.h"
#include "test_util.h"

void app_main( void ) {
    sntp_init_before_wifi();
    nvs_init();
    gpio_init();
    gpio_logic_init();
    rest_init();
    sntp_init_after_wifi();
//    test_init();
}
