#ifndef HAJO_N2K_PROTOCOL_H
#define HAJO_N2K_PROTOCOL_H

#define FASTPACKET_INDEX (0)
#define FASTPACKET_SIZE (1)
#define FASTPACKET_BUCKET_0_SIZE (6)
#define FASTPACKET_BUCKET_N_SIZE (7)
#define FASTPACKET_BUCKET_0_OFFSET (2)
#define FASTPACKET_BUCKET_N_OFFSET (1)
#define FASTPACKET_MAX_INDEX (0x1f)
#define FASTPACKET_MAX_SIZE (FASTPACKET_BUCKET_0_SIZE + FASTPACKET_BUCKET_N_SIZE * FASTPACKET_MAX_INDEX)

typedef struct {
    uint8_t prio;
    uint32_t pgn;
    uint8_t dst;
    uint8_t src;
    uint8_t len;
    uint8_t data[FASTPACKET_MAX_SIZE];
} can_message_t;

// receiver has to free the memory
typedef void (*n2k_callback)( const can_message_t *message );

extern void n2k_main();

extern void n2k_send( const can_message_t *message );

extern void n2k_register_receiver( n2k_callback receiver );


#endif //HAJO_N2K_PROTOCOL_H
