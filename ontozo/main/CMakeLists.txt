idf_component_register(
        SRCS
        "../../common/debug_helper.c"
        "../../common/gpio_define.c"
        "../../common/gpio_json.c"
        "../../common/gpio_task.c"
        "../../common/http/http_discovery.c"
        "../../common/http/http_server.c"
        "../../common/http/http_static_file.c"
        "../../common/main_main.c"
        "../../common/nvs_main.c"
        "../../common/rest_util.c"
        "../../common/sntp_main.c"
        "../../common/system_info_rest.c"
        "../../common/wifi/connect.c"
        "../../common/wifi/scan.c"
        "../../common/wifi/wifi_connect.c"
        "../../common/wifi/wifi_main.c"
        "config_version.c"
        "gpio_logic.c"
        "gpio_rest.c"
        "program.c"
        "program_json.c"
        "program_logic.c"
        "program_logic_rest.c"
        "program_start.c"
        "program_rest.c"
        "ontozo_main.c"
        "rest_handler.c"
        "test_util.c"
        INCLUDE_DIRS
        "."
        "../../common"
)

if (CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend")
    if (EXISTS ${WEB_SRC_DIR}/dist/frontend/browser)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist/frontend/browser FLASH_IN_PROJECT)
    else ()
        message(FATAL_ERROR "${WEB_SRC_DIR}/dist/frontend/browser doesn't exit. Please run 'ng build' in ${WEB_SRC_DIR}")
    endif ()
endif ()
