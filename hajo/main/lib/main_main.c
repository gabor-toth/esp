#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "main_main.h"

void main_main() {
    ESP_ERROR_CHECK( nvs_flash_init());
    ESP_ERROR_CHECK( esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default());
}