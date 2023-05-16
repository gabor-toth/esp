#include "esp_vfs_fat.h"
#include "unistd.h"
#include "sys/stat.h"
#include "cstring"
#include "sdmmc_cmd.h"
#include "n2k_parser.h"
#include "n2k_sender.h"

static const char *TAG = "sdcard";

#define MOUNT_POINT "/sdcard"

#define PIN_NUM_MISO  GPIO_NUM_39
#define PIN_NUM_MOSI  GPIO_NUM_41
#define PIN_NUM_CLK   GPIO_NUM_40
#define PIN_NUM_CS    GPIO_NUM_42

static void open_sdcard() {
    esp_err_t ret;

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
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI( TAG, "Initializing SD card" );

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI( TAG, "Using SPI peripheral" );

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = PIN_NUM_MISO,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = SDMMC_FREQ_DEFAULT,
            .flags = 0,
            .isr_cpu_id = INTR_CPU_ID_AUTO,
            .intr_flags = 0
    };
    ret = spi_bus_initialize( static_cast<spi_host_device_t>(host.slot), &bus_cfg, SDSPI_DEFAULT_DMA );
    if ( ret != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to initialize bus." );
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

    ESP_LOGI( TAG, "Mounting filesystem" );
    ret = esp_vfs_fat_sdspi_mount( mount_point, &host, &slot_config, &mount_config, &card );

    if ( ret != ESP_OK ) {
        if ( ret == ESP_FAIL ) {
            ESP_LOGE( TAG, "Failed to mount filesystem. "
                           "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option." );
        } else {
            ESP_LOGE( TAG, "Failed to initialize the card (%s). "
                           "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name( ret ));
        }
        return;
    }
    ESP_LOGI( TAG, "Filesystem mounted" );

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card );

    // Use POSIX and C standard library functions to work with files.

    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";

    ESP_LOGI( TAG, "Opening file %s", file_hello );
    FILE *f = fopen( file_hello, "w" );
    if ( f == nullptr ) {
        ESP_LOGE( TAG, "Failed to open file for writing" );
        return;
    }
    fprintf( f, "Hello %s!\n", card->cid.name );
    fclose( f );
    ESP_LOGI( TAG, "File written" );

    const char *file_foo = MOUNT_POINT"/foo.txt";

    // Check if destination file exists before renaming
    struct stat st{};
    if ( stat( file_foo, &st ) == 0 ) {
        // Delete it if it exists
        unlink( file_foo );
    }

    // Rename original file
    ESP_LOGI( TAG, "Renaming file %s to %s", file_hello, file_foo );
    if ( rename( file_hello, file_foo ) != 0 ) {
        ESP_LOGE( TAG, "Rename failed" );
        return;
    }

    // Open renamed file for reading
    ESP_LOGI( TAG, "Reading file %s", file_foo );
    f = fopen( file_foo, "r" );
    if ( f == nullptr ) {
        ESP_LOGE( TAG, "Failed to open file for reading" );
        return;
    }

    // Read a line from file
    char line[64];
    fgets( line, sizeof( line ), f );
    fclose( f );

    // Strip newline
    char *pos = strchr( line, '\n' );
    if ( pos ) {
        *pos = '\0';
    }
    ESP_LOGI( TAG, "Read from file: '%s'", line );

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount( mount_point, card );
    ESP_LOGI( TAG, "Card unmounted" );

    //deinitialize the bus after all devices are removed
    spi_bus_free( static_cast<spi_host_device_t>(host.slot));
}

/*
 * 0x1F801: PGN 129025 - Position, Rapid Update (100msec)
 * 0x1F805: PGN 129029 - GNSS Position Data (1000msec)
 * 0x1F809: PGN 129033 - Time & Date (1000msec)
 */
static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                        // N2kVersion
            100,                        // Manufacturer's product code
            "Data logger",               // Manufacturer's Model ID
            "0.1.0 (2023-03-23)",        // Manufacturer's Software version code
            "1.0.0 (2023-03-23)",    // Manufacturer's Model version
            "00000001",            // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                         // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=Bus Traffic Logger
                                   10,        // Device class=System Tools
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

void hajo_logger_main( int iDev) {
    open_sdcard();
    setup_n2k_device( iDev );
}
