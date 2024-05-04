#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "driver/gpio.h"
#include "config.h"
#include "esp_log.h"
#include "sdcard.h"
#include "gpio_define.h"
#include "n2k/n2k_struct_parser.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_util.h"

#define LED_TIME_ON 20
#define LED_TIME_GAP 200
#define LED_TIME_INTERVAL 2000

#define BUTTONS_CLASS 0

typedef enum {
    led_mode_off,
    led_mode_green,
    led_mode_green_red,
    led_mode_red,
} led_mode_t;

static volatile led_mode_t led_mode = led_mode_off;
static volatile bool logging_on = false;

static const char *LOG = "hajo_logger";

/*
 * 0x1F801: PGN 129025 - Position, Rapid Update (100msec)
 * 0x1F805: PGN 129029 - GNSS Position Data (1000msec)
 * 0x1F809: PGN 129033 - Time & Date (1000msec)
 * 0ff63
 * 0x1EF00: proprietary fast packet
 * 0x1F10D: PGN 127245 - Rudder
 */
class LoggerIncomingMessageHandler : public tNMEA2000::tMsgHandler {
public:
    explicit LoggerIncomingMessageHandler( tNMEA2000 *_pNMEA2000 ) : tNMEA2000::tMsgHandler( 0, _pNMEA2000 ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg ) override;
};

static LoggerIncomingMessageHandler *incomingMessageHandler;

static void process_incoming_pgn_gnss_position_data( const tN2kMsg &msg ) {
    N2kGNSSData data;
    if ( ParseN2kGNSS( msg, data )) {
        ESP_LOGI( LOG, "PGN position data latitude %lf longitude %lf sats %d type %d method %d",
                  data.latitude, data.longitude, data.satellites, data.gnssType, data.gnssMethod );
    }
}

static void process_incoming_pgn_local_offset( const tN2kMsg &msg ) {
    N2kLocalOffsetData data;
    if ( ParseN2kLocalOffset( msg, data )) {
        ESP_LOGI( LOG, "PGN local offset days %d seconds %lf offset %d",
                  data.daysSince1970, data.secondsSinceMidnight, data.localOffset );
    }
}

static void process_incoming_pgn_rudder( const tN2kMsg &msg ) {
    N2kRudderData data;
    static int counter = 0;
    if ( ++counter < 10 ) {
        return;
    }
    counter = 0;
    if ( ParseN2kRudder( msg, data )) {
    }
}

__attribute__((unused))
static void process_incoming_pgn_proprietary_fast_packet( const tN2kMsg &msg ) {
    int index = 0;
    int vb = msg.Get2ByteUInt( index );
    int manufacturerCode = vb & (( 1 << 12 ) - 1 );
    int industryCode = vb >> 12;
    int proprietaryId = msg.Get2ByteUInt( index );
    int command = msg.GetByte( index );
    ESP_LOGI( LOG, "PGN proprietary manu %04x industry %d proprietaryId %04x/%d command %02d/%d",
              manufacturerCode, industryCode,
              proprietaryId, proprietaryId,
              command, command );
}

// 3b 07     raymarine 73B
// 3b 1f     2x1 reserved
// 3b 9f     marine 4
// f0 81 84  SeaTalk Pilot Mode
// f0 81 86  SeaTalk Keystroke
// f0 81 ae  ?
// proprietary_fast_packet:
// 3b 9f f0 81 ae 02 00 08 00
// 3b 9f f0 81 84 06 00 00 00 00 00 03 06
// N2K_PGN_RAYMARINE_PILOT_MODE:
// 3b 9f 00 00 02 00 01 ff

//I (225529) hajo_logger: PGN local offset days 12326 seconds 76950.000000 offset 32767
//I (225539) hajo_logger: PGN position data latitude 47.580750 longitude 19.060717 sats 4 type 0 method 1

__attribute__((unused))
static void process_incoming_pgn_dump( const tN2kMsg &msg ) {
    char buf[16 * 3 + 1];
    char *p = buf;
    for ( int i = 0; i < msg.DataLen; i++, p += 3 ) {
        sprintf( p, "%02x ", msg.Data[ i ] );
    }
    ESP_LOGI( LOG, "PGN %5lx len %2d data %s", msg.PGN, msg.DataLen, buf );
}

void LoggerIncomingMessageHandler::HandleMsg( const tN2kMsg &N2kMsg ) {
//    ESP_LOGI( LOG, "PGN %5lx len %2d", N2kMsg.PGN, N2kMsg.DataLen );
    switch ( N2kMsg.PGN ) {
//        case N2K_PGN_FLUID_LEVEL:
//            process_incoming_pgn_fluid_level( N2kMsg );
//            break;
//        case N2K_PGN_BATTERY_STATUS:
//            process_incoming_pgn_battery_status( N2kMsg );
//            break;
//        case N2K_PGN_DC_DETAILED_STATUS:
//            process_incoming_pgn_dc_detailed_status( N2kMsg );
//            break;
//        case N2K_PGN_BATTERY_CONFIGURATION:
//            process_incoming_pgn_battery_configuration( N2kMsg );
//            break;
        case N2K_PGN_GNSS_POSITION_DATA:
            process_incoming_pgn_gnss_position_data( N2kMsg );
            break;
        case N2K_PGN_LOCAL_OFFSET:
            process_incoming_pgn_local_offset( N2kMsg );
            break;
        case N2K_PGN_RUDDER:
            process_incoming_pgn_rudder( N2kMsg );
            break;
        case N2K_PGN_PROPRIETARY_FAST_PACKET:
//            process_incoming_pgn_proprietary_fast_packet( N2kMsg );
//            process_incoming_pgn_dump( N2kMsg );
            break;
        case 0xFF63: //N2K_PGN_RAYMARINE_PILOT_MODE:
//            process_incoming_pgn_dump( N2kMsg );
            break;
    }
}

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
    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
                                   140,    // Device function=Bus Traffic Logger
                                   10,        // Device class=System Tools
                                   2046,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                   4,       // Marine
                                   iDev
    );
//    NMEA2000.SetDeviceInformation( n2k_get_device_id(),      // Unique number. Use e.g. Serial number.
//                                   131,    // Device function=NMEA 2000 to Analog Gateway
//                                   25,        // Inter/Intranetwork Device
//                                   2047,  // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
//                                   4,       // Marine
//                                   iDev
//    );

    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
//    NMEA2000.SetMode( tNMEA2000::N2km_ListenOnly, NodeAddress );

    NMEA2000.ExtendTransmitMessages( TransmitMessages, iDev );
    NMEA2000.ExtendReceiveMessages( ReceiveMessages, iDev );
    incomingMessageHandler = new LoggerIncomingMessageHandler( &NMEA2000 );
    NMEA2000.AttachMsgHandler( incomingMessageHandler );
}

static void process_incoming_pgn( const tN2kMsg &message ) {
    incomingMessageHandler->HandleMsg( message );
}

_Noreturn static void task_main_test_sdcard( void *arg ) {
    (void) arg;
    
    while (true) {
        if ( logging_on ) {
            if ( test_sdcard() ) {
                led_mode = led_mode_green_red;
            } else {
                led_mode = led_mode_red;
            }
        } else {
            led_mode = led_mode_green;
        }
        vTaskDelay(pdMS_TO_TICKS( 5000 ));
    }
}

_Noreturn static void task_logger_led( void *arg ) {
    (void) arg;
    
    TickType_t flashMarker = 0;
    for ( ;; ) {
        gpio_num_t led1 = GPIO_NUM_NC;
        gpio_num_t led2 = GPIO_NUM_NC;
        switch(led_mode) {
            case led_mode_off:
                vTaskDelayUntil(&flashMarker, pdMS_TO_TICKS(LED_TIME_INTERVAL) );
                continue;
            case led_mode_green:
                led1 = led2 = GPIO_NUM_LOGGER_LED_GREEN;
                break;
            case led_mode_green_red:
                led1 = GPIO_NUM_LOGGER_LED_GREEN;
                led2 = GPIO_NUM_LOGGER_LED_RED;
                break;
            case led_mode_red:
                led1 = led2 = GPIO_NUM_LOGGER_LED_RED;
                break;
        }
        gpio_set_level(led1, 1 );
        vTaskDelayUntil(&flashMarker, pdMS_TO_TICKS(LED_TIME_ON) );
        gpio_set_level(led1, 0 );
        vTaskDelayUntil(&flashMarker, pdMS_TO_TICKS(LED_TIME_GAP) );
        gpio_set_level(led2, 1 );
        vTaskDelayUntil(&flashMarker, pdMS_TO_TICKS(LED_TIME_ON) );
        gpio_set_level(led2, 0 );
        vTaskDelayUntil(&flashMarker, pdMS_TO_TICKS(LED_TIME_INTERVAL-2*LED_TIME_ON-LED_TIME_GAP) );
    }
}

void gpio_define_output_pins_callback( gpio_config_t *io_conf, void *user_context ) {
}

void gpio_define_input_pins_callback( gpio_config_t *io_conf, void *user_context ) {
    gpio_add_class( INPUTS, "buttons", 2, low_is_on );
    
    gpio_add_pin( INPUTS, BUTTONS_CLASS, GPIO_NUM_LOGGER_BUTTON_1,
                  low_is_on, &io_conf->pin_bit_mask );
    gpio_add_pin( INPUTS, BUTTONS_CLASS, GPIO_NUM_LOGGER_BUTTON_2,
                  low_is_on, &io_conf->pin_bit_mask );
}

void gpio_changed_callback( gpio_num_t io_num, int state ) {
    if ( io_num == GPIO_NUM_LOGGER_BUTTON_1 ) {
        if ( state == 0 ) {
            logging_on = !logging_on;
            ESP_LOGI( LOG, "Logging turned %s", logging_on ? "on" : "off" );
            led_mode = logging_on ? led_mode_green_red : led_mode_green;
        }
//    } else if ( io_num == GPIO_NUM_LOGGER_BUTTON_2 ) {
//        if ( state == 0 ) {
//        }
    } else {
        ESP_LOGW( LOG, "Unhandled gpio %d changed to %d", io_num, state );
    }
}

static void init_leds_and_buttons() {
    gpio_set_direction(GPIO_NUM_LOGGER_LED_GREEN, GPIO_MODE_OUTPUT );
    gpio_set_direction(GPIO_NUM_LOGGER_LED_RED, GPIO_MODE_OUTPUT );
    gpio_set_level(GPIO_NUM_LOGGER_LED_GREEN, 0 );
    gpio_set_level(GPIO_NUM_LOGGER_LED_RED, 0 );
    
    gpio_init( nullptr );
    xTaskCreate( task_logger_led, "logger_led", 1024, nullptr, 10, nullptr );
}

void hajo_logger_main( int iDev ) {
    logging_on = false;
    init_leds_and_buttons();
    
    setup_n2k_device( iDev );

    n2k_sender_register_loopback( process_incoming_pgn );
    
    xTaskCreate( task_main_test_sdcard, "test_sdcard", 4096, nullptr, 5, nullptr );
}
