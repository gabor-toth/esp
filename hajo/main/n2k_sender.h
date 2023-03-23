#ifndef HAJO_N2K_SENDER_H
#define HAJO_N2K_SENDER_H

#include "NMEA2000.h"
#include "N2kMessages.h"

extern tNMEA2000 &NMEA2000;

typedef bool (*tN2kSendFunction)( int index, tN2kMsg &message );

void nk2_register_sender( tN2kSendFunction sendFunction,
                          const char *description,
                          uint32_t periodMs,
                          uint32_t offsetMs,
                          bool enabled );

extern void n2k_onOpen();

#endif //HAJO_N2K_SENDER_H
