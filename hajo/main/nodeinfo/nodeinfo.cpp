#include "nodeinfo.h"
#include "esp_log.h"
#include "../n2k/n2k_sender.h"

#define MANUFACTURER_CODE_OWN ((1<<12)-1)
#define INDUSTRY_CODE_MARINE (4)
#define PROPRIETARY_ID_WON 33264xxxx

#define MANUFACTURER_INDUSTRY_OWN ((uint16_t)((INDUSTRY_CODE_MARINE<<13)|(0<11)|(MANUFACTURER_CODE_OWN)))

void SetN2kPGN65410( tN2kMsg &N2kMsg ) {
    N2kMsg.SetPGN( 65410L );
    N2kMsg.Add2ByteUInt( MANUFACTURER_INDUSTRY_OWN );
    N2kMsg.AddByte( 1 ); //
    N2kMsg.Add2ByteUInt( N2kUInt16NA ); // Internal Device Temperature
    N2kMsg.Add2ByteUInt( N2kUInt16NA ); // supply voltage
    N2kMsg.AddByte( 0 );  // reserved
}

static bool send_node_info( int index, tN2kMsg &message, int &deviceIndex ) {
    if ( index != 0 ) {
        return false;
    }
    SetN2kPGN65410( message );
    return true;
}

void node_info_main() {
    nk2_register_sender( send_node_info, "node_info", 1000, 98, true );
}
