#set(IDF_COMPONENT_MANAGER 0)

set(SOURCES
        "../../common/adc.c"
        "../../common/debug_helper.c"
        "../../common/gpio_define.c"
        "../../common/gpio_json.c"
        "../../common/gpio_task.c"
        "../../common/logger.c"
        "../../common/main_main.c"
        "../../common/nvs_main.c"
        "../../common/http/http_discovery.c"
        "../../common/http/http_server.c"
        "../../common/http/http_static_file.c"
        "../../common/rest_util.c"
        "../../common/sntp_main.c"
        "../../common/system_info_rest.c"
        "../../common/wifi/connect.c"
        "../../common/wifi/scan.c"
        "../../common/wifi/wifi_connect.c"
        "../../common/wifi/wifi_main.c"
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
        "devices/fridge/fridge.cpp"
        "devices/fluid/hajo_fluid.cpp"
        "devices/logger/hajo_logger.cpp"
        "devices/logger/sdcard.c"
        "devices/signalk/EspSigK.cpp"
        "devices/signalk/HttpRequest.cpp"
        "devices/signalk/NMEA2000-SignalK-Gateway.cpp"
        "devices/signalk/hajo_signalk.cpp"
        "devices/signalk/signalk_rest.c"
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
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "BATTERY" is_device_type_battery)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "DISPLAY" is_device_type_display)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "FRIDGE" is_device_type_fridge)
    string(COMPARE EQUAL ${CONFIG_DEVICE_TYPE} "GATEWAY" is_device_type_gateway)
    if (is_device_type_battery)
        add_compile_definitions(CONFIG_DEVICE_TYPE_BATTERY=1)
    elseif (is_device_type_fridge)
        add_compile_definitions(CONFIG_DEVICE_TYPE_FRIDGE=1)
    elseif (is_device_type_display)
        add_compile_definitions(CONFIG_DEVICE_TYPE_DISPLAY=1)
    elseif (is_device_type_gateway)
        add_compile_definitions(CONFIG_DEVICE_TYPE_GATEWAY=1)
    else ()
        message(FATAL_ERROR "Unknown CONFIG_DEVICE_TYPE ${CONFIG_DEVICE_TYPE}, should be BATTERY/DISPLAY/FRIDGE/GATEWAY")
    endif ()
    #else ()
    #message(FATAL_ERROR "Set CONFIG_DEVICE_TYPE like 'idf.py build -DCONFIG_DEVICE_TYPE=GATEWAY'")
endif ()
UNSET(is_device_type_battery CACHE)
UNSET(is_device_type_display CACHE)
UNSET(is_device_type_fridge CACHE)
UNSET(is_device_type_gateway CACHE)
