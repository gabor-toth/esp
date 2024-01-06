idf_build_get_property(target IDF_TARGET)

#if (${target} STREQUAL "linux")
#    # Header only library for linux
#
#    idf_component_register(INCLUDE_DIRS include
#            SRCS protocol_examples_utils.c)
#    return()
#endif ()

set(srcs
        "adc.c"
        "gpio_define.c"
        "gpio_json.c"
        "gpio_task.c"
        "logger.c"
        "main_main.c"
        "mdns_main.c"
        "nvs_main.c"
        "rest_main.c"
        "rest_server.c"
        "rest_util.c"
        "simple_list.c"
        "sntp_main.c"
        "system_info_rest.c"
        "timer.c"
        "wifi/wifi_connect.c"
        "wifi/connect.c"
        "wifi/wifi_main.c"
        "ws_keep_alive.c"
        "ws_server.c"
)

idf_component_register(
        SRCS "${srcs}"
        INCLUDE_DIRS "include"

        PRIV_REQUIRES
        driver
        esp_adc
        esp_event
        esp_http_server
        esp_hw_support
        esp_netif
        esp_timer
        esp_wifi
        espressif__mdns
        json
        nvs_flash
        spiffs
        vfs
)

#    idf_component_optional_requires(PRIVATE esp_eth)
