#ifndef HAJO_N2K_GATEWAY_H
#define HAJO_N2K_GATEWAY_H

#include "N2kMsg.h"

extern void setupSignalkChannels();

extern void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg );

#endif //HAJO_N2K_GATEWAY_H
