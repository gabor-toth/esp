#ifndef HAJO_N2K_GATEWAY_WIFI_H
#define HAJO_N2K_GATEWAY_WIFI_H

#include "N2kMsg.h"

extern void setupSignalkOverWifi();

extern void sendN2KMessageToSignalKOverWifi( const tN2kMsg &N2kMsg );

#endif //HAJO_N2K_GATEWAY_WIFI_H
