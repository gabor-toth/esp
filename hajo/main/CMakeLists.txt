idf_component_register(
        SRCS
        "fonts/rubik_12.c"
        "fonts/rubik_12_subpx.c"
        "fonts/rubik_18.c"
        "fonts/rubik_18_subpx.c"
        "lib/adc.c"
        "lib/gpio_define.c"
        "lib/gpio_task.c"
        "lib/main_main.c"
        "lib/nvs_main.c"
        "lib/rest_util.c"
        "lib/sntp_main.c"
        "lib/system_info_rest.c"
        "lib/wifi_connect.c"
        "lib/gpio_json.c"
        "display.c"
        "hajo_adc.c"
        "hajo_main.c"
        "n2k_protocol.c"
        "n2k_sender.c"
        INCLUDE_DIRS
        "."
        "lib"
)

if (CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend ")
    if (EXISTS ${WEB_SRC_DIR}/dist/frontend)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist/frontend FLASH_IN_PROJECT)
    else ()
        message(FATAL_ERROR " ${WEB_SRC_DIR}/dist/frontend doesn't exit. Please run 'ng build' in ${WEB_SRC_DIR} ")
    endif ()
endif ()
