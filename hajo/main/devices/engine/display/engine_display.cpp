#include "driver/gpio.h"
#include "engine_display.h"
#include "engine_display_internal.h"
#include "engine_display_draw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"

static const char *TAG = "display";

static int myDeviceIndex;
static TimerHandle_t flashTimer;
static TimerHandle_t idleTimer;
display_data_t displayData;

/*
 * PNG to XBM:
 * - open PNG in Gimp
 * - Image/Mode/Indexed: choose black&white
 * - Image/Resize image: lock aspect ratio, set size to 16
 * - File/Export: change extension to .xbm
 * - Copy file content here
 */

static void process_incoming_pgn( const tN2kMsg &message );

static void setup_n2k_device( int iDev ) {
    static const unsigned long TransmitMessages[] = {
            0
    };

    static const unsigned long ReceiveMessages[] = {
            N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
            N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
            0
    };

    static const tNMEA2000::tProductInformation ProductInformation = {
            2100,                    // N2kVersion
            107,                     // Manufacturer's product code
            "Engine display",      // Manufacturer's Model ID
            "1.0.0 (2025-11-22)",    // Manufacturer's Software version code
            "1.0.0 (2025-11-22)",    // Manufacturer's Model version
            "00000001",              // Manufacturer's Model serial code
            0,                       // CertificationLevel
            1                        // LoadEquivalency
    };

    NMEA2000.SetProductInformation( &ProductInformation, iDev );

    // device class & function: https://manualzz.com/doc/12647142/nmea2000-class-and-function-codes
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   130,    // Display
                                   120,        // Device class=Display
                                   2046,  // Just chosen free from code list on https://github.com/ieb/EngineMonitor/blob/master/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                   4,       // Marine
                                   iDev
    );
    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    NMEA2000.AttachMsgHandler( new N2kIncomingMessageHandler( &NMEA2000, process_incoming_pgn ) );
}

static void process_engine_rapid_pgn( const tN2kMsg &N2kMsg ) {
    N2kEngineParamRapid data;
    if ( !ParseN2kEngineParamRapid( N2kMsg, data ) ) {
        return;
    }
    bool changed = false;
    n2k_incoming_value( (int16_t) data.engineSpeedRpm, displayData.rpm, changed );
    if ( changed ) {
        engine_display_draw_screen();
    }
}

static void set_flash() {
    ESP_LOGI(TAG,"failure state oil=%d cooling=%d charger=%d",
             displayData.oilPressureFailure,
             displayData.coolingWaterTemperatureFailure,
             displayData.chargerFailure );
    bool hasFailure = displayData.oilPressureFailure
            || displayData.coolingWaterTemperatureFailure
            ||displayData.chargerFailure;
    if ( hasFailure != displayData.hasFailure ) {
        if ( hasFailure ) {
            // turn flash on
            displayData.flashState = true;
            xTimerStart( flashTimer, portMAX_DELAY );
        } else {
            // turn flash off
            xTimerStop( flashTimer, portMAX_DELAY );
            displayData.flashState = false;
        }
        ESP_LOGW(TAG,"flash state %d", displayData.flashState);
        displayData.hasFailure = hasFailure;
    }
}

static void process_engine_dynamic_pgn( const tN2kMsg &N2kMsg ) {
    N2kEngineDynamicParam data;
    if ( !ParseN2kEngineDynamicParam( N2kMsg, data ) ) {
        return;
    }
    bool changed = false;
    n2k_incoming_value( (int16_t) data.engineHours, displayData.hours, changed );
    n2k_incoming_value( data.engineOilPress < 0.0, displayData.oilPressureFailure, changed );
    n2k_incoming_value( data.engineCoolantTemp < 0.0, displayData.coolingWaterTemperatureFailure, changed );
    n2k_incoming_value( data.alternatorVoltage < 0.0, displayData.chargerFailure, changed );
    if ( changed ) {
        set_flash();
        engine_display_draw_screen();
    }
}

static void reset_idle_timer() {
    xTimerReset( idleTimer, portMAX_DELAY );
}

static void process_incoming_pgn( const tN2kMsg &message ) {
    switch ( message.PGN ) {
        case N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE:
            process_engine_rapid_pgn( message );
            reset_idle_timer();
            break;
        case N2K_PGN_ENGINE_PARAMETERS_DYNAMIC:
            process_engine_dynamic_pgn( message );
            reset_idle_timer();
            break;
    }
}

static void send_test_pngs() {
    tN2kMsg messageRapid;
    SetN2kPGN127488( messageRapid, 1, 6789 );
    process_incoming_pgn( messageRapid );

    tN2kMsg messageDynamic;
    SetN2kPGN127489( messageDynamic, 1, N2kDoubleNA, 0.0, 0.0, 0.0, N2kDoubleNA, 6789.0 );
    process_incoming_pgn( messageDynamic );
}

void set_initial_display_data() {
    displayData.rpm = displayData.hours = (int16_t )N2kDoubleNA;
    displayData.chargerFailure = displayData.oilPressureFailure = displayData.coolingWaterTemperatureFailure = false;
}

static void flash_timer_callback( TimerHandle_t xTimer ) {
    displayData.flashState = !displayData.flashState;
    engine_display_draw_screen();
}

static void idle_timer_callback( TimerHandle_t xTimer ) {
    set_initial_display_data();
    set_flash();
    engine_display_draw_screen();
}

void engine_display_main( int iDev ) {
    gpio_config_t gpioConfig;
    gpioConfig.pin_bit_mask = 1 << PIN_BACKLIGHT;
    gpioConfig.mode = GPIO_MODE_OUTPUT;
    gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpioConfig.intr_type = GPIO_INTR_DISABLE;
    gpio_config( &gpioConfig );

    myDeviceIndex = iDev;
    setup_n2k_device( iDev );

    set_initial_display_data();
    displayData.hasFailure = false;

    engine_display_setup_display();
    engine_display_draw_screen();

    flashTimer = xTimerCreate(
            TAG,
            pdMS_TO_TICKS( 500 ),
            1,
            nullptr,
            flash_timer_callback );
    idleTimer = xTimerCreate(
            TAG,
            pdMS_TO_TICKS( 5000 ),
            1,
            nullptr,
            idle_timer_callback );
    xTimerStart( idleTimer, portMAX_DELAY );

    send_test_pngs();
}

void engine_display_test() {
    engine_display_setup_display();
    displayData.rpm = 6789;
    displayData.hours = 6789;
    displayData.chargerFailure = false;
    displayData.oilPressureFailure = true;
    displayData.coolingWaterTemperatureFailure = false;
    engine_display_draw_screen();
}
