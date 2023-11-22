#set(IDF_COMPONENT_MANAGER 0)

set(DEVICE_TYPE_ALL 99)
set(DEVICE_TYPE_LOGGER 1)
set(DEVICE_TYPE_GAUGE_DISPLAY 5)
set(DEVICE_TYPE_BATTERY_MONITOR 6)

set(DEVICE_TYPE ${DEVICE_TYPE_ALL})
#set(DEVICE_TYPE ${DEVICE_TYPE_LOGGER})

set(SOURCES
        "lib/gpio_define.c"
        "lib/gpio_task.c"
        "lib/main_main.c"
        "lib/nvs_main.c"
        "hajo_main.cpp"
        "n2k/n2k_struct_parser.cpp"
        "n2k/n2k_receiver.cpp"
        "n2k/n2k_sender.cpp"
        "n2k/n2k_util.cpp"
        "n2k/N2kRaymarine.cpp"
)

if ((DEVICE_TYPE EQUAL DEVICE_TYPE_LOGGER) OR (DEVICE_TYPE EQUAL DEVICE_TYPE_ALL))
    set(SOURCES ${SOURCES}
            "devices/attitude/hajo_attitude.cpp"
            "devices/logger/hajo_logger.cpp"
            "devices/logger/sdcard.c"
            "devices/signalk/EspSigK.cpp"
            "devices/signalk/hajo_signalk.cpp"
            "devices/signalk/NMEA2000-SignalK-Gateway.cpp"
            "lib/gpio_json.c"
            "lib/logger.c"
            "lib/rest_main.c"
            "lib/rest_server.c"
            "lib/rest_util.c"
            "lib/sntp_main.c"
            "lib/system_info_rest.c"
            "lib/wifi/connect.c"
            "lib/wifi/wifi_connect.c"
            "lib/ws_keep_alive.c"
            "lib/ws_server.c"
    )
endif ()

if ((DEVICE_TYPE EQUAL DEVICE_TYPE_GAUGE_DISPLAY) OR (DEVICE_TYPE EQUAL DEVICE_TYPE_ALL))
    set(SOURCES ${SOURCES}
            "fonts/rubik_12.c"
            "fonts/rubik_12_subpx.c"
            "fonts/rubik_18.c"
            "fonts/rubik_18_subpx.c"
            "devices/display/display_main.c"
            "devices/display/display_meter_single.c"
            "devices/display/display_meter_tabbed.c"
            "devices/display/hajo_display.cpp"
            "devices/fluid/hajo_fluid.cpp"
            "lib/adc.c"
    )
endif ()

if ((DEVICE_TYPE EQUAL DEVICE_TYPE_BATTERY_MONITOR) OR (DEVICE_TYPE EQUAL DEVICE_TYPE_ALL))
    set(SOURCES ${SOURCES}
            "devices/battery/hajo_battery.cpp"
            "lib/adc.c"
    )
endif ()

idf_component_register(
        SRCS ${SOURCES}

        INCLUDE_DIRS
        "."
        "lib"

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
