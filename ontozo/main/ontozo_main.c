#include "lib/gpio_define.h"
#include "test_util.h"
#include "lib/rest_main.h"
#include "lib/nvs_main.h"

void app_main( void ) {
    nvs_init();
    gpio_init();
    rest_init();
//    test_init();
}
