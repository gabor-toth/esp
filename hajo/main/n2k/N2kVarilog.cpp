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

bool ParseN2kPGNVarilogEngineKeyPress( const tN2kMsg &N2kMsg, uint8_t &instanceId, uint8_t& sid, N2kVarilogEngineKeys &keysPressed, N2kVarilogEngineKeys& keysChanged ) {
    if ( N2kMsg.PGN != N2K_PGN_VARILOG_ENGINE_KEY_PRESS ) {
        return false;
    }

    int Index = 0;
    int manufacturerIndustryCode = N2kMsg.Get2ByteUInt( Index );
    if ( manufacturerIndustryCode != VARILOG_MANUFACTURER_INDUSTRY ) {
        return false;
    }
    instanceId = N2kMsg.GetByte( Index );
    sid = N2kMsg.GetByte( Index );
    keysPressed.ByteValue = N2kMsg.GetByte( Index );
    keysChanged.ByteValue = N2kMsg.GetByte( Index );

    return true;
}

void SetN2kPGNVarilogEngineKeyPress( tN2kMsg &N2kMsg, uint8_t instanceId, uint8_t sid, N2kVarilogEngineKeys keysPressed, N2kVarilogEngineKeys keysChanged ) {
    N2kMsg.SetPGN( N2K_PGN_VARILOG_ENGINE_KEY_PRESS );
    N2kMsg.Add2ByteUInt( VARILOG_MANUFACTURER_INDUSTRY );
    N2kMsg.AddByte( instanceId );
    N2kMsg.AddByte( sid );
    N2kMsg.AddByte( keysPressed.ByteValue );
    N2kMsg.AddByte( keysChanged.ByteValue );
    // fill to 8 bytes
    N2kMsg.AddByte( 0 );
    N2kMsg.AddByte( 0 );
}

bool ParseN2kPGNVarilogEngineKeyPressAck( const tN2kMsg &N2kMsg, uint8_t &instanceId, uint8_t& sid ) {
    if ( N2kMsg.PGN != N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK ) {
        return false;
    }

    int Index = 0;
    int manufacturerIndustryCode = N2kMsg.Get2ByteUInt( Index );
    if ( manufacturerIndustryCode != VARILOG_MANUFACTURER_INDUSTRY ) {
        return false;
    }
    instanceId = N2kMsg.GetByte( Index );
    sid = N2kMsg.GetByte( Index );

    return true;
}

void SetN2kPGNVarilogEngineKeyPressAck( tN2kMsg &N2kMsg, uint8_t instanceId, uint8_t sid ) {
    N2kMsg.SetPGN( N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK );
    N2kMsg.Add2ByteUInt( VARILOG_MANUFACTURER_INDUSTRY );
    N2kMsg.AddByte( instanceId );
    N2kMsg.AddByte( sid );
    // fill to 8 bytes
    N2kMsg.AddByte( 0 );
    N2kMsg.AddByte( 0 );
    N2kMsg.AddByte( 0 );
    N2kMsg.AddByte( 0 );
}
