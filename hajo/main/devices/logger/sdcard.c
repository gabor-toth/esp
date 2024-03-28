#include "sdcard.h"
#include "config.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "string.h"
#include "unistd.h"
#include "sys/stat.h"

static const char *LOG = "sdcard";

#define MOUNT_POINT "/sdcard"

bool test_sdcard() {
    esp_err_t ret;
    bool result = false;
    
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .format_if_mount_failed = true,
#else
            .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
            .disk_status_check_enable = false
    };
    sdmmc_card_t *card = NULL;
    ESP_LOGI( LOG, "Initializing SD card" );
    
    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI( LOG, "Using SPI peripheral" );
    
    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
//    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    
    spi_bus_config_t bus_cfg = {
            .mosi_io_num = GPIO_NUM_SDCARD_MOSI,
            .miso_io_num = GPIO_NUM_SDCARD_MISO,
            .sclk_io_num = GPIO_NUM_SDCARD_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = 0,
            .flags = 0,
            .isr_cpu_id = INTR_CPU_ID_AUTO,
            .intr_flags = 0
    };
    ret = spi_bus_initialize( host.slot, &bus_cfg, SDSPI_DEFAULT_DMA );
    if ( ret != ESP_OK ) {
        ESP_LOGE( LOG, "Failed to initialize bus." );
        return false;
    }
    
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_SDCARD_CS;
    slot_config.host_id = host.slot;
    
    ESP_LOGI( LOG, "Mounting filesystem" );
    ret = esp_vfs_fat_sdspi_mount( MOUNT_POINT, &host, &slot_config, &mount_config, &card );
    
    if ( ret != ESP_OK ) {
        if ( ret == ESP_FAIL ) {
            ESP_LOGE( LOG, "Failed to mount filesystem. "
                           "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option." );
        } else {
            ESP_LOGE( LOG, "Failed to initialize the card (%s). "
                           "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name( ret ) );
        }
        goto err;
    }
    ESP_LOGI( LOG, "Filesystem mounted" );
    
    // Card has been initialized, print its properties
    sdmmc_card_print_info( stdout, card );
    
    // Use POSIX and C standard library functions to work with files.
    
    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";
    
    ESP_LOGI( LOG, "Opening file %s", file_hello );
    FILE *f = fopen( file_hello, "w" );
    ESP_LOGI( LOG, "Opened" );
    if ( f == NULL ) {
        ESP_LOGE( LOG, "Failed to open file for writing" );
        goto err;
    }
    ESP_LOGI( LOG, "Writing" );
    fprintf( f, "Hello %s!\n", card->cid.name );
    ESP_LOGI( LOG, "Written" );
    fclose( f );
    ESP_LOGI( LOG, "File written" );
    
    const char *file_foo = MOUNT_POINT"/foo.txt";
    
    // Check if destination file exists before renaming
    struct stat st;
    if ( stat( file_foo, &st ) == 0 ) {
        // Delete it if it exists
        unlink( file_foo );
    }
    
    // Rename original file
    ESP_LOGI( LOG, "Renaming file %s to %s", file_hello, file_foo );
    if ( rename( file_hello, file_foo ) != 0 ) {
        ESP_LOGE( LOG, "Rename failed" );
        goto err;
    }
    
    // Open renamed file for reading
    ESP_LOGI( LOG, "Reading file %s", file_foo );
    f = fopen( file_foo, "r" );
    if ( f == NULL ) {
        ESP_LOGE( LOG, "Failed to open file for reading" );
        goto err;
    }
    
    // Read a line from file
    char line[64];
    fgets( line, sizeof(line), f );
    fclose( f );
    
    // Strip newline
    char *pos = strchr( line, '\n' );
    if ( pos ) {
        *pos = '\0';
    }
    ESP_LOGI( LOG, "Read from file: '%s'", line );
    
    result = true;
    
    err:
    if ( card != NULL ) {
        // All done, unmount partition and disable SPI peripheral
        esp_vfs_fat_sdcard_unmount( MOUNT_POINT, card );
        ESP_LOGI( LOG, "Card unmounted" );
    }
    
    // deinitialize the bus after all devices are removed
    spi_bus_free( host.slot );
    ESP_LOGI( LOG, "Card freed" );
    
    return result;
}
