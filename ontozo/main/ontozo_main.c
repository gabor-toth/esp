#include "gpio_define.h"
#include "gpio_logic.h"
#include "http/http_server.h"
#include "main_main.h"
#include "nvs_main.h"
#include "program.h"
#include "program_logic.h"
#include "program_start.h"
#include "sntp_main.h"
#include "wifi/wifi_main.h"

void app_main( void ) {
    main_main();

    nvs_init();
    gpio_logic_init();
    program_init();

    sntp_init_before_wifi();
    rest_init_before_wifi();

//    wifi_connect();

//    rest_init_after_wifi();
//    sntp_init_after_wifi();

    program_logic_init();
    program_start_init();
}
