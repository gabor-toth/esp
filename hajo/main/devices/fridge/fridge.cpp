#include "esp_log.h"
#include "esp_timer.h"
#include "fridge.h"
#include "n2k/n2k_sender.h"
#include "n2k/N2kVarilog.h"

#define DO_ANIMATION 0

static const char *TAG = "fridge";


#if DO_ANIMATION
#define ANIMATION_SECS   3

static double duty_cycle;
static bool ascending;
static int animation_counter;
#endif

static int myDeviceIndex;

static void timer_callback( void *arg ) {
    fridge_fan_timer_handler();
    fridge_temp_timer_handler();

#if DO_ANIMATION
    if ( --animation_counter == 0 ) {
        animation_counter = ANIMATION_SECS;
        duty_cycle += 0.05 * ( ascending ? 1 : -1 );
        if ( duty_cycle >= 1.0 ) {
            ascending = false;
            duty_cycle = 1.0;
        } else if ( duty_cycle <= 0.0 ) {
            ascending = true;
            duty_cycle = 0.0;
        }
    }

    fridge_fan_set_duty_cycle( 0, duty_cycle );
    ESP_LOGI( TAG, "Duty %0.2lf",  duty_cycle );
#endif
}

static void setup_fridge_timer() {
    esp_timer_create_args_t tca = {
            .callback = (esp_timer_cb_t) timer_callback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = nullptr,
            .skip_unhandled_events = false
    };

    esp_timer_handle_t timer = nullptr;
    esp_err_t stat = esp_timer_create( &tca, &timer );
    if ( stat != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to create timer, err 0x%x\n", stat );
        return;
    }
    esp_timer_start_periodic( timer, 1000000L );
}

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            N2K_PGN_RUDDER,
            0
    };

    static const unsigned long ReceiveMessages[] = {
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                    // N2kVersion
            101,                     // Manufacturer's product code
            "Fridge extension",      // Manufacturer's Model ID
            "1.0.0 (2024-09-24)",    // Manufacturer's Software version code
            "1.0.0 (2024-09-24)",    // Manufacturer's Model version
            "00000001",              // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                        // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   130,    // Temperature
                                   75,        // Device class=Sensor Communication Interface
                                   N2K_MANUFACTURER_CODE_VARILOG,
                                   4,       // Marine
                                   iDev
    );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
}

static bool send_temperature( int index, tN2kMsg &message, int& deviceIndex ) {
    deviceIndex = myDeviceIndex;
    if ( index > 0 ) {
        return false;
    }

//    adc_channel_value_t channel_data;
//    adc_get_channel_value( 0, &channel_data );
//
//    SetN2kTemperature( message,
//                  channel_data.display_value != INT_MIN ? DegToRad(channel_data.display_value/(double)display_scale) :  N2kDoubleNA , // radians
//                  0, // instance
//                  N2kRDO_NoDirectionOrder,
//                  N2kDoubleNA // angleOrder
//    );
    return true;
}

void fridge_main( int iDev ) {
    myDeviceIndex = iDev;
    fridge_fan_setup();
    fridge_temp_setup();
    setup_n2k_device( iDev );

    nk2_register_sender( send_temperature, "fridge", N2K_PGN_TEMPERATURE_INTERVAL_MS*10, 65, true );

#if DO_ANIMATION
    duty_cycle = 0.0;
    ascending = true;
    animation_counter = ANIMATION_SECS;
#endif

    setup_fridge_timer();
}
