#include "mdns_main.h"
#include "lwip/err.h"
#include "mdns.h"
#include "rest_server.h"
#include "wifi/wifi_main.h"

static void initialise_mdns( void ) {
    ESP_ERROR_CHECK( mdns_init());
    esp_netif_t *netif = wifi_get_esp_netif();
    if ( netif != NULL) {
        const char *hostname;
        ESP_ERROR_CHECK( esp_netif_get_hostname( netif, &hostname ));
        ESP_ERROR_CHECK( mdns_hostname_set( hostname ));
    } else {
        ESP_ERROR_CHECK( mdns_hostname_set( CONFIG_EXAMPLE_MDNS_HOST_NAME ));
    }
    ESP_ERROR_CHECK( mdns_instance_name_set( CONFIG_MDNS_INSTANCE_NAME ));

    mdns_txt_item_t serviceTxtData[] = {
            { "board", "esp32s2" },
            { "path",  "/" }
    };

    ESP_ERROR_CHECK( mdns_service_add(
            CONFIG_MDNS_INSTANCE_NAME,
            "_http",
            "_tcp",
            80,
            serviceTxtData,
            sizeof( serviceTxtData ) / sizeof( serviceTxtData[ 0 ] )));
    /*
    ESP_ERROR_CHECK( mdns_service_subtype_add_for_host(
            CONFIG_MDNS_INSTANCE_NAME,
            "_http",
            "_tcp",
            NULL,
            "_server" ));
    */
}

static err_enum_t on_wifi_connect(  const char *wifi_ssid ) {
    (void) wifi_ssid;

    initialise_mdns();
    return ESP_OK;
}

static void on_wifi_disconnect( httpd_handle_t server ) {
    (void) server;

    mdns_free();
}

static const wifi_callbacks_t callbacks = {
        .name= "rest-main",
        .wifi_connect_fn = on_wifi_connect,
        .wifi_disconnect_fn= on_wifi_disconnect,
};

void mdns_register() {
    wifi_register_callbacks( &callbacks );
}
