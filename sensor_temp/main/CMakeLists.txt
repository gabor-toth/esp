idf_component_register(
        SRCS
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

SITE_NAME(_host_name)
string(COMPARE EQUAL ${_host_name} "tothgnb4" is_bare_hardware)
add_compile_definitions(CONFIG_BARE_HARDWARE=${is_bare_hardware})
#message("-- added CONFIG_BARE_HARDWARE=${is_bare_hardware}")
UNSET(_host_name CACHE)
UNSET(is_bare_hardware CACHE)
