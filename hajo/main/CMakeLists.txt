#set(IDF_COMPONENT_MANAGER 0)

set(DEVICE_TYPE_ALL 99)
set(DEVICE_TYPE_LOGGER 1)
set(DEVICE_TYPE_GAUGE_DISPLAY 5)
set(DEVICE_TYPE_BATTERY_MONITOR 6)

set(DEVICE_TYPE ${DEVICE_TYPE_ALL})
#set(DEVICE_TYPE ${DEVICE_TYPE_LOGGER})

set(SOURCES
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
            "devices/signalk/HttpRequest.cpp"
            "devices/signalk/hajo_signalk.cpp"
            "devices/signalk/NMEA2000-SignalK-Gateway.cpp"
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
    )
endif ()

if ((DEVICE_TYPE EQUAL DEVICE_TYPE_BATTERY_MONITOR) OR (DEVICE_TYPE EQUAL DEVICE_TYPE_ALL))
    set(SOURCES ${SOURCES}
            "devices/battery/hajo_battery.cpp"
    )
endif ()

idf_component_register(
        SRCS ${SOURCES}

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

SITE_NAME(_host_name)
string(COMPARE EQUAL ${_host_name} "tothgnb4" is_bare_hardware)
add_compile_definitions(CONFIG_BARE_HARDWARE=${is_bare_hardware})
#message("-- added CONFIG_BARE_HARDWARE=${is_bare_hardware}")
UNSET(_host_name CACHE)
UNSET(is_bare_hardware CACHE)
