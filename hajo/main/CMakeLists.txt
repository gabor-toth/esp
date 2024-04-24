#set(IDF_COMPONENT_MANAGER 0)

set(SOURCES
        "../../common/adc.c"
        "../../common/gpio_define.c"
        "../../common/gpio_json.c"
        "../../common/gpio_task.c"
        "../../common/logger.c"
        "../../common/main_main.c"
        "../../common/nvs_main.c"
        "../../common/rest_main.c"
        "../../common/rest_server.c"
        "../../common/rest_util.c"
        "../../common/sntp_main.c"
        "../../common/system_info_rest.c"
        "../../common/wifi/connect.c"
        "../../common/wifi/wifi_connect.c"
        "../../common/ws_keep_alive.c"
        "../../common/ws_server.c"
        "assets/img_battery.c"
        "assets/img_battery_engine.c"
        "assets/img_fuel.c"
        "assets/img_water.c"
        "devices/attitude/hajo_attitude.cpp"
        "devices/battery/hajo_battery.cpp"
        "devices/display/display_main.c"
        "devices/display/display_meter.c"
        "devices/display/hajo_display.cpp"
        "devices/fluid/hajo_fluid.cpp"
        "devices/logger/hajo_logger.cpp"
        "devices/logger/sdcard.c"
        "devices/signalk/EspSigK.cpp"
        "devices/signalk/HttpRequest.cpp"
        "devices/signalk/NMEA2000-SignalK-Gateway.cpp"
        "devices/signalk/hajo_signalk.cpp"
        "devices/wap/wap_main.c"
        "fonts/rubik_12.c"
        "fonts/rubik_12_subpx.c"
        "fonts/rubik_18.c"
        "fonts/rubik_18_subpx.c"
        "n2k/N2kRaymarine.cpp"
        "n2k/n2k_receiver.cpp"
        "n2k/n2k_sender.cpp"
        "n2k/n2k_struct_parser.cpp"
        "n2k/n2k_util.cpp"
        "hajo_main.cpp"
)

idf_component_register(
        SRCS ${SOURCES}

        INCLUDE_DIRS
        "."
        "../../common"

        # REQUIRES
        # "driver"
        # "esp_adc"
        # "esp_event"
        # "json"
        # "lvgl"
        # "NMEA2000"
        # "nvs_flash"
)

if (CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../frontend ")
    if (EXISTS ${WEB_SRC_DIR}/dist/frontend)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist/frontend FLASH_IN_PROJECT)
    else ()
        message(FATAL_ERROR " ${WEB_SRC_DIR}/dist/frontend doesn't exit. Please run 'ng build' in ${WEB_SRC_DIR} ")
    endif ()
endif ()

if (CONFIG_DEVICE_TYPE)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "BATTERY" is_device_type_battery_monitor)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "LOGGER" is_device_type_logger)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "FLUID" is_device_type_gauge_display)
    if (is_device_type_logger)
        add_compile_definitions(CONFIG_DEVICE_TYPE_LOGGER=1)
    elseif (is_device_type_gauge_display)
        add_compile_definitions(CONFIG_DEVICE_TYPE_GAUGE_DISPLAY=1)
    elseif (is_device_type_battery_monitor)
        add_compile_definitions(CONFIG_DEVICE_TYPE_BATTERY_MONITOR=1)
    else ()
        message(FATAL_ERROR "Unknown CONFIG_DEVICE_TYPE ${CONFIG_DEVICE_TYPE}, should be BATTERY/FLUID/LOGGER")
    endif ()
    #else ()
    #message(FATAL_ERROR "Set CONFIG_DEVICE_TYPE like 'idf.py build -DCONFIG_DEVICE_TYPE=LOGGER'")
endif ()
UNSET(is_device_type_battery_monitor CACHE)
UNSET(is_device_type_logger CACHE)
UNSET(is_device_type_gauge_display CACHE)
