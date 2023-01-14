#ifndef HAJO_N2K_SENDER_H
#define HAJO_N2K_SENDER_H

#include "n2k_protocol.h"
#include "stdbool.h"

typedef bool (*n2k_sender_callback)( int index, can_message_t *message );

extern void nk2_register_sender( const char *name, int interval_ms, n2k_sender_callback callback );

#endif //HAJO_N2K_SENDER_H
