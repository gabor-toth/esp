#ifndef HAJO_N2K_RECEIVER_H
#define HAJO_N2K_RECEIVER_H

#include "NMEA2000.h"

extern void n2k_init();

typedef void (*N2kIncomingMessageCallback)( const tN2kMsg &N2kMsg );

class N2kIncomingMessageHandler : public tNMEA2000::tMsgHandler {
private:
    N2kIncomingMessageCallback callback;
public:
    explicit N2kIncomingMessageHandler( tNMEA2000 *_pNMEA2000, N2kIncomingMessageCallback callback ) :
            tNMEA2000::tMsgHandler( 0, _pNMEA2000 ),
            callback( callback ) {
    }

    void HandleMsg( const tN2kMsg &N2kMsg ) override;
};

#endif //HAJO_N2K_RECEIVER_H
