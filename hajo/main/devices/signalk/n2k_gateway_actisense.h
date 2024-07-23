#ifndef HAJO_N2K_GATEWAY_ACTISENSE_H
#define HAJO_N2K_GATEWAY_ACTISENSE_H

#include "N2kMsg.h"

extern void setupSignalkOverActisense();

extern void sendN2KMessageToSignalKOverActisense( const tN2kMsg &N2kMsg );

#endif //HAJO_N2K_GATEWAY_ACTISENSE_H
