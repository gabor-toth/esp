#include "n2k_util.h"
#include "esp_mac.h"

uint32_t n2k_get_device_id() {
    // Generate unique number from chip id
    uint8_t chipId[6];
    esp_efuse_mac_get_default( chipId );
    uint32_t id = 0;
    for ( int i = 0; i < 6; i++ ) {
        id += ( chipId[ i ] << ( 7 * i ) );
    }
    return id;
}
