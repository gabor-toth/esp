#include "n2k_gateway.h"
#include "n2k_simulator.h"
#include "sdkconfig.h"

#if CONFIG_SIGNALK_OVER_ACTISENSE

#include "n2k_gateway_actisense.h"

#endif
#if CONFIG_SIGNALK_OVER_WIFI

#include "n2k_gateway_wifi.h"

#endif

void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg ) {
#if CONFIG_SIGNALK_OVER_ACTISENSE
    sendN2KMessageToSignalKOverActisense( N2kMsg );
#endif
#if CONFIG_SIGNALK_OVER_WIFI
    sendN2KMessageToSignalKOverWifi( N2kMsg );
#endif
}

void setupSignalkChannels() {
#if CONFIG_SIGNALK_OVER_ACTISENSE
    setupSignalkOverActisense();
#endif
#if CONFIG_SIGNALK_OVER_WIFI
    setupSignalkOverWifi();
#endif
#if CONFIG_SIGNALK_START_SIMULATOR
    pngSimulationStart();
#endif
}
