idf_component_register(
        SRCS
        "lib/gpio_define.c"
        "lib/gpio_task.c"
        "lib/main_main.c"
        "lib/nvs_main.c"
        "lib/rest_util.c"
        "lib/sntp_main.c"
        "lib/system_info_rest.c"
        "lib/wifi_connect.c"
        "lib/gpio_json.c"
        "hajo_main.c"
        INCLUDE_DIRS
        "."
        "lib"
)

if (CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend")
    if (EXISTS ${WEB_SRC_DIR}/dist/frontend)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist/frontend FLASH_IN_PROJECT)
    else ()
        message(FATAL_ERROR "${WEB_SRC_DIR}/dist/frontend doesn't exit. Please run 'ng build' in ${WEB_SRC_DIR}")
    endif ()
endif ()
