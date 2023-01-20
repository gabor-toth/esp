#include "config.h"
#include <driver/twai.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/timers.h>
#include "n2k_protocol.h"
#include "n2k_png.h"
#include <string.h>

static const char *LOG = "can";

static QueueHandle_t receive_event_queue = NULL;
static QueueHandle_t transmit_event_queue = NULL;
static n2k_callback receiver_callback = NULL;
static n2k_callback receiver_sender_loopback = NULL;
static uint8_t source_id = 0;
static int receive_timeout_secs = 30;
static bool log_message_header = false;

static void log_packet( const can_message_t *message, bool out ) {
    char data[2 * TWAI_FRAME_MAX_DLC + 1];
    char *dst = data;
    for ( int i = 0; i < message->len; i++, dst += 2 ) {
        sprintf( dst, "%02x", message->data[ i ] );
    }
    ESP_LOGI( LOG, "%c PGN %06lx src %02x dst %02x prio %02x len %2d data %s",
              ( out ? '>' : '<' ),
              message->pgn, message->src, message->dst, message->prio, message->len,
              data );
}

static void sendN2kFastPacket( can_message_t *message, twai_message_t *frame ) {
    int index = 0;
    int remainingDataBytes = message->len;
    while ( remainingDataBytes > 0 ) {
        frame->data[ 0 ] = index; // fast packet index (increases by 1 for every CAN frame), 'order' (the 3 uppermost bits) is left as 0 for now

        if ( index == 0 ) {
            // 1st frame
            frame->data[ 1 ] = message->len;             // fast packet payload size
            memcpy( frame->data + 2, message->data, 6 ); // 6 first data bytes
            frame->data_length_code = 8;
            remainingDataBytes -= 6;
        } else {
            // further frames
            if ( remainingDataBytes > 7 ) {
                memcpy( frame->data + 1, message->data + 6 + ( index - 1 ) * 7, 7 ); // 7 next data bytes
                frame->data_length_code = 8;
                remainingDataBytes -= 7;
            } else {
                memcpy( frame->data + 1, message->data + 6 + ( index - 1 ) * 7,
                        remainingDataBytes ); // 7 next data bytes
                frame->data_length_code = 1 + remainingDataBytes;
                remainingDataBytes = 0;
            }
        }
        log_packet( message, true );
        esp_err_t result = twai_transmit( frame, 10 );
        if ( ESP_OK != result ) {
            ESP_LOGW( LOG, "twai_transmit resulted in %d", result );
        }
        index++;
    }
}

unsigned int getCanIdFromISO11783Bits( unsigned int prio, unsigned int pgn, unsigned int src, unsigned int dst ) {
    // src bits are the lowest ones of the CAN ID. Also set the highest bit to 1 as n2k uses
    unsigned int canId = ( src & 0xff ) | 0x80000000U;
    // only extended frames (EFF bit).
    unsigned int PF = ( pgn >> 8 ) & 0xff;

    if ( PF < 240 ) {
        // PDU 1
        canId |= ( dst & 0xff ) << 8;
        canId |= ( pgn << 8 );
    } else {
        // PDU 2
        canId |= pgn << 8;
    }
    canId |= prio << 26;

    if ( log_message_header ) {
        ESP_LOGI( LOG, "getCanIdFromISO11783Bits(%08x,%02x,%05x,%02x,%02x)", canId,
                  prio,
                  pgn,
                  src,
                  dst );
    }
    return canId;
}

static void sendN2kPacket( can_message_t *message ) {
    twai_message_t frame;
    memset( &frame, 0, sizeof( frame ));

    if ( message->pgn >= ( 1 << 18 )) {
        // PGNs can't have more than 18 bits, otherwise it overwrites priority bits
        ESP_LOGE( LOG, "Invalid PGN, too big (0x%lx). Skipping.\n", message->pgn );
        return;
    }

    if ( receiver_sender_loopback != NULL) {
        receiver_sender_loopback( message );
    }

    uint8_t source = source_id; // msg->src
    frame.identifier = getCanIdFromISO11783Bits( message->prio, message->pgn, source, message->dst );
    frame.extd = true;

    if ( message->len <= 8 ) {
        // 8 or less bytes of data -> PGN fits into a single CAN frame
        frame.data_length_code = message->len;
        memcpy( frame.data, message->data, message->len );
        log_packet( message, true );
        esp_err_t result = twai_transmit( &frame, 10 );
        if ( ESP_OK != result ) {
            ESP_LOGW( LOG, "twai_transmit resulted in %d", result );
        }
    } else {
        // Send PGN as n2k fast packet (spans multiple CAN frames, but CAN ID is still same for each frame)
        sendN2kFastPacket( message, &frame );
    }
}

void initialize_driver() {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << N2K_GPIO_NUM_STANDBY;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config( &io_conf );
    gpio_set_level( N2K_GPIO_NUM_STANDBY, 0 );

    //Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT( N2K_GPIO_NUM_TX, N2K_GPIO_NUM_RX, TWAI_MODE_NO_ACK );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK( twai_driver_install( &g_config, &t_config, &f_config ));
    ESP_ERROR_CHECK( twai_start());
}

#undef PS

void getISO11783BitsFromCanId( uint32_t id, uint8_t *prio, uint32_t *pgn, uint8_t *src, uint8_t *dst ) {
    unsigned char PF = (unsigned char) ( id >> 16 );
    unsigned char PS = (unsigned char) ( id >> 8 );
    unsigned char RDP = (unsigned char) ( id >> 24 ) & 3; // Use R + DP bits

    if ( src ) {
        *src = (unsigned char) id >> 0;
    }
    if ( prio ) {
        *prio = (unsigned char) (( id >> 26 ) & 0x7 );
    }

    if ( PF < 240 ) {
        /* PDU1 format, the PS contains the destination address */
        if ( dst ) {
            *dst = PS;
        }
        if ( pgn ) {
            *pgn = ( RDP << 16 ) + ( PF << 8 );
        }
    } else {
        /* PDU2 format, the destination is implied global and the PGN is extended */
        if ( dst ) {
            *dst = 0xff;
        }
        if ( pgn ) {
            *pgn = ( RDP << 16 ) + ( PF << 8 ) + PS;
        }
    }
}

static bool is_standalone_packet( twai_message_t *message, can_message_t *can_message ) {
    getISO11783BitsFromCanId( message->identifier,
                              &can_message->prio,
                              &can_message->pgn,
                              &can_message->src,
                              &can_message->dst );
    uint32_t pgn = can_message->pgn;
    if ( log_message_header ) {
        ESP_LOGI( LOG, "getISO11783BitsFromCanId(%08lx,%02x,%05lx,%02x,%02x)", message->identifier,
                  can_message->prio,
                  pgn,
                  can_message->src,
                  can_message->dst );
    }
    if ( pgn <= 0XFFFF ) {
        return true;
    }
    if ( pgn < 0x1F000 || pgn >= 0x1FF00 ) {
        return false;
    }
    // TODO PDU2 (non-addressed) mixed single/fast packet PGN range 0x1F000 to 0x1FEFF (126976 - 130815)
    switch ( pgn ) {
        case N2K_PGN_FLUID_LEVEL: // 0x1F211
        case N2K_PGN_BATTERY_STATUS: // 0x1F214
            return true;
        default:
            return false;
    }
}

static void packet_received( const can_message_t *can_message ) {
    log_packet( can_message, false );
    if ( receiver_callback == NULL) {
        return;
    }
    receiver_callback( can_message );
}

static void receive_standalone_packet( const twai_message_t *twai_message, can_message_t *can_message ) {
    can_message->len = twai_message->data_length_code;
    memcpy( &can_message->data, twai_message->data, twai_message->data_length_code );
    packet_received( can_message );
}

static void receive_fast_packet( twai_message_t *message, can_message_t *can_message ) {
    ESP_LOGW( LOG, "fast packets not yet implemented " );
}

static void receive_packet( twai_message_t *twai_message ) {
    can_message_t can_message;
    // is_standalone_packet fills the fields prio, pgn, dst, src
    if ( is_standalone_packet( twai_message, &can_message )) {
        receive_standalone_packet( twai_message, &can_message );
    } else {
        receive_fast_packet( twai_message, &can_message );
    }
}

_Noreturn static void receive_task_main( void *arg ) {
    (void) arg;

    twai_message_t message;
    for ( ;; ) {
        esp_err_t result = twai_receive( &message, pdMS_TO_TICKS( receive_timeout_secs * 1000 ));
        if ( result == ESP_ERR_TIMEOUT ) {
            ESP_LOGI( LOG, "nothing received in %d secs ", receive_timeout_secs );
            continue;
        }
        receive_packet( &message );
    }
}

_Noreturn static void transmit_task_main( void *arg ) {
    (void) arg;

    for ( ;; ) {
        can_message_t can_message;
        if ( xQueueReceive( transmit_event_queue, &can_message, portMAX_DELAY )) {
            sendN2kPacket( &can_message );
        }
    }
}

void create_event_task() {
    receive_event_queue = xQueueCreate( 10, sizeof( uint32_t ));
    transmit_event_queue = xQueueCreate( 10, sizeof( can_message_t ));
    xTaskCreate( receive_task_main, "canrx", 3072, NULL, 5, NULL);
    xTaskCreate( transmit_task_main, "cantx", 3072, NULL, 5, NULL);
}

void n2k_main() {
    ESP_LOGI( LOG, "n2k start" );
    initialize_driver();
    create_event_task();
    ESP_LOGI( LOG, "n2k started" );
}

void n2k_send( const can_message_t *message ) {
    xQueueSend( transmit_event_queue, message, 0 );
}

void n2k_register_receiver( n2k_callback receiver ) {
    receiver_callback = receiver;
}

void n2k_register_sender_loopback( n2k_callback receiver ) {
    receiver_sender_loopback = receiver;
}

static_assert( sizeof( pgn_iso_address_claim_t ) == 8, "Size of pgn_iso_address_claim_t is not correct" );
