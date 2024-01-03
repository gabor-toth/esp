idf_component_register(
        SRCS
        "../../common/adc.c"
        "../../common/gpio_define.c"
        "../../common/gpio_task.c"
        "../../common/main_main.c"
        "../../common/nvs_main.c"
        "../../common/wifi/connect.c"
        "../../common/wifi/wifi_connect.c"
        "sensor_main.cpp"

        INCLUDE_DIRS
        "."
        "../../common"
)

if (CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend ")
    if (EXISTS ${WEB_SRC_DIR}/dist/frontend)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist/frontend FLASH_IN_PROJECT)
    else ()
        message(FATAL_ERROR " ${WEB_SRC_DIR}/dist/frontend doesn't exit. Please run 'ng build' in ${WEB_SRC_DIR} ")
    endif ()
endif ()
