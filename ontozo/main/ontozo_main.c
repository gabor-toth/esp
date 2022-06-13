#include "lib/gpio_define.h"
#include "lib/main_main.h"
#include "lib/nvs_main.h"
#include "lib/rest_main.h"
#include "lib/sntp_main.h"
#include "lib/wifi_connect.h"
#include "gpio_logic.h"
#include "program.h"
#include "program_logic.h"

void app_main( void ) {
    main_main();

    nvs_init();
    gpio_logic_init();
    program_init();

    sntp_init_before_wifi();
    rest_init_before_wifi();

    wifi_connect();

    rest_init_after_wifi();
    sntp_init_after_wifi();

    program_logic_init();
}
