#include "N2kVarilog.h"

#define VARILOG_MANUFACTURER_INDUSTRY ((uint16_t)((4<<13)|(0<11)|(N2K_MANUFACTURER_CODE_VARILOG)))

/*

 0xEF00: Manufacturer Proprietary single-frame addressed
 0xEF00: PGN 61184 - Seatalk: Wireless Keypad Control

 1	Manufacturer Code	1851: Raymarine     0 .. 2044       11 bits lookup MANUFACTURER_CODE
 2	Reserved			                                    2 bits RESERVED
 3	Industry Code       4: Marine Industry  0 .. 6          3 bits lookup INDUSTRY_CODE
 4	Proprietary ID                          0 .. 252        8 bits unsigned NUMBER	true

 0xFF00-0xFFFF: Manufacturer Proprietary single-frame non-addressed
*/

bool
ParseN2kPGNVarilogEngineKeyPress( const tN2kMsg &N2kMsg, uint8_t &instanceId, N2kVarilogEngineKeyPress &keyPress ) {
    if ( N2kMsg.PGN != N2K_PGN_VARILOG_ENGINE_KEY_PRESS ) {
        return false;
    }

    int Index = 0;
    int manufacturerIndustryCode = N2kMsg.Get2ByteUInt( Index );
    if ( manufacturerIndustryCode != VARILOG_MANUFACTURER_INDUSTRY ) {
        return false;
    }
    instanceId = N2kMsg.GetByte( Index );
    keyPress = static_cast<N2kVarilogEngineKeyPress>(N2kMsg.GetByte( Index ));

    return true;
}

void SetN2kPGNVarilogEngineKeyPress( tN2kMsg &N2kMsg, uint8_t instanceId, N2kVarilogEngineKeyPress keyPress ) {
    N2kMsg.SetPGN( N2K_PGN_VARILOG_ENGINE_KEY_PRESS );
    N2kMsg.Add2ByteUInt( VARILOG_MANUFACTURER_INDUSTRY );
    N2kMsg.AddByte( instanceId );
    N2kMsg.AddByte( keyPress );
}
