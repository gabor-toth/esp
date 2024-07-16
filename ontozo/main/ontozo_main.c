#include "debug_helper.h"
#include "gpio_logic.h"
#include "http/http_discovery.h"
#include "http/http_server.h"
#include "http/http_static_file.h"
#include "main_main.h"
#include "nvs_main.h"
#include "program.h"
#include "program_logic.h"
#include "program_start.h"
#include "rest_handler.h"
#include "sntp_main.h"
#include "wifi/wifi_main.h"

void app_main( void ) {
    debug_start_heap_dump(10);
    main_main();

    nvs_init();
    gpio_logic_init();
    program_init();

    sntp_init_before_wifi();
    discovery_register();

    http_server_main(DEFAULT_HTTP_SERVER_CONTEXT_SIZE);
    http_static_files_register();
    rest_register();
    ESP_ERROR_CHECK( wifi_main() );

//    rest_init_after_wifi();
//    sntp_init_after_wifi();

    program_logic_init();
    program_start_init();
}
