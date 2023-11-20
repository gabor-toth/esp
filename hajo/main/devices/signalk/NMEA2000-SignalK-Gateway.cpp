/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Read PGNs from NMEA2000-Bus and send as SignalK to server
// Version 0.1, 06.02.2021, AK-Homberger

#include "esp_log.h"
#include "N2kMessages.h"
#include "N2kMsg.h"
#include "N2kTypes.h"

#include "devices/signalk/EspSigK.h"        // For SignalK handling

static const char* TAG = "n2k-gw";

// Set the information for other bus devices, which messages we support
//const unsigned long ReceiveMessages[] PROGMEM = {/*126992L,*/ // System time
//      127250L, // Heading
//      128259L, // Boat speed
//      128267L, // Depth
//      129025L, // Position
//      129026L, // COG and SOG
//      129029L, // GNSS
//      130306L, // Wind
//      128275L, // Log
//      130310L, // Water temperature
//      127245L, // Rudder
//      0
//    };



//*****************************************************************************
void HandleHeading( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kHeadingReference ref;
    double Deviation = 0;
    double Variation;
    double Heading;

    if ( ParseN2kHeading( N2kMsg, SID, Heading, Deviation, Variation, ref )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.headingTrue", Heading + Variation + Deviation );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleBoatSpeed( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    double WaterReferenced;
    double GroundReferenced;
    tN2kSpeedWaterReferenceType SWRT;

    if ( ParseN2kBoatSpeed( N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.speedThroughWater", WaterReferenced );
        sigK.addDeltaValue( "navigation.speedOverGround", GroundReferenced );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleDepth( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    double DepthBelowTransducer;
    double Offset;
    double Range;
    double WaterDepth;

    if ( ParseN2kWaterDepth( N2kMsg, SID, DepthBelowTransducer, Offset, Range )) {
        WaterDepth = DepthBelowTransducer + Offset;
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "environment.depth.belowTransducer", DepthBelowTransducer );
        sigK.addDeltaValue( "environment.depth.belowSurface", WaterDepth );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandlePosition( const tN2kMsg &N2kMsg ) {
    double Latitude;
    double Longitude;
    char buf[100];

    if ( ParseN2kPGN129025( N2kMsg, Latitude, Longitude )) {
        snprintf( buf, sizeof( buf ), "{\"altitude\":%f,\"latitude\":%f,\"longitude\":%f}", 0.0, Latitude, Longitude );
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.position", buf );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleCOG_SOG( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kHeadingReference ref;
    double COG;
    double SOG;

    if ( ParseN2kPGN129026( N2kMsg, SID, ref, COG, SOG )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.courseOverGroundTrue", COG );
        sigK.addDeltaValue( "navigation.speedOverGround", SOG );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleWind( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kWindReference WindReference;

    double WindAngle, WindSpeed;

    if ( ParseN2kWindSpeed( N2kMsg, SID, WindSpeed, WindAngle, WindReference )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        if ( WindReference == N2kWind_Apparent ) {
            sigK.addDeltaValue( "environment.wind.angleApparent", WindAngle );
            sigK.addDeltaValue( "environment.wind.speedApparent", WindSpeed );
        } else if ( WindReference == N2kWind_True_boat ) {
            sigK.addDeltaValue( "environment.wind.angleTrueGround", WindAngle );
            sigK.addDeltaValue( "environment.wind.speedTrue", WindSpeed );
        } else if ( WindReference == N2kWind_True_water ) {
            sigK.addDeltaValue( "environment.wind.angleTrueWater", WindAngle );
            sigK.addDeltaValue( "environment.wind.speedTrue", WindSpeed );
        }
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleLog( const tN2kMsg &N2kMsg ) {

    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    uint32_t Log;
    uint32_t TripLog;

    if ( ParseN2kDistanceLog( N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.trip.log", (int) TripLog );
        sigK.addDeltaValue( "navigation.log", (int) Log );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleWaterTemp( const tN2kMsg &N2kMsg ) {

    unsigned char SID;
    double OutsideAmbientAirTemperature;
    double AtmosphericPressure;
    double WaterTemperature;

    if ( ParseN2kPGN130310( N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        // sigK.addDeltaValue("environment.outside.temperature", OutsideAmbientAirTemperature);
        // sigK.addDeltaValue("environment.outside.pressure", AtmosphericPressure);
        sigK.addDeltaValue( "environment.water.temperature", WaterTemperature );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleRudder( const tN2kMsg &N2kMsg ) {

    double RudderPosition;
    unsigned char Instance;
    tN2kRudderDirectionOrder RudderDirectionOrder;
    double AngleOrder;

    if ( ParseN2kRudder( N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder )) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "steering.rudderAngle", RudderPosition );
        sigK.sendDelta();
    }
}


//*****************************************************************************
void HandleGNSS( const tN2kMsg &N2kMsg ) {

    unsigned char SID;
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    double Latitude;
    double Longitude;
    double Altitude;
    tN2kGNSStype GNSSType;
    tN2kGNSSmethod GNSSMethod;
    unsigned char nSatellites;
    double HDOP;
    double PDOP;
    double GeoidalSeparation;
    unsigned char nReferenceStations;
    tN2kGNSStype ReferenceStationType;
    uint16_t ReferenceStationID;
    double AgeOfCorrection;
    char buf[100];

    if ( ParseN2kGNSS( N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSSType,
                       GNSSMethod,
                       nSatellites, HDOP, PDOP, GeoidalSeparation,
                       nReferenceStations, ReferenceStationType, ReferenceStationID, AgeOfCorrection )) {

        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.gnss.type", GNSSType );
        sigK.addDeltaValue( "navigation.gnss.horizontalDilution", HDOP );
        sigK.addDeltaValue( "navigation.gnss.positionDilution", PDOP );

//        sigK.sendDelta();

        sigK.addDeltaValue( "navigation.gnss.satellites", nSatellites );
        sigK.addDeltaValue( "navigation.gnss.geoidalSeparation", GeoidalSeparation );
        sigK.addDeltaValue( "navigation.gnss.differentialAge", AgeOfCorrection );

//        sigK.sendDelta();

        sigK.addDeltaValue( "navigation.gnss.differentialReference", ReferenceStationID );
        snprintf( buf, sizeof( buf ), R"({"altitude":%f,"latitude":%f,"longitude":%f})", Altitude, Latitude,
                  Longitude );
        sigK.addDeltaValue( "navigation.position", buf );

        sigK.sendDelta();
    }
}


void HandleAttitude( const tN2kMsg &N2kMsg ) {
    unsigned char sid ;
    double yaw;
    double pitch;
    double roll ;

    if ( ParseN2kAttitude( N2kMsg, sid, yaw, pitch, roll)) {
        sigK.startDelta(N2kMsg.Source, N2kMsg.PGN);
        sigK.addDeltaValue( "navigation.attitude.pitch", pitch );
        sigK.addDeltaValue( "navigation.attitude.roll", roll );
        sigK.addDeltaValue( "navigation.attitude.yaw", yaw );
        sigK.sendDelta();
    }
}

bool ParseN2kPGN126720(const tN2kMsg &N2kMsg, uint16_t& ManufacturerCode, uint8_t& Reserved, uint8_t&  IndustryCode, uint16_t& ProprietaryID, uint8_t& Command) {
    if (N2kMsg.PGN!=126720L) return false;

    int Index=0;
    uint16_t v = N2kMsg.Get2ByteUInt(Index);
    ManufacturerCode = v & ((1<<11)-1);
    Reserved = (v>>11) & 0x03;
    IndustryCode = (v>>13) & 0x07;
    ProprietaryID =  N2kMsg.Get2ByteUInt(Index);
    Command =  N2kMsg.GetByte(Index);

    return true;
}

void HandleProprietaryFastPacket( const tN2kMsg &N2kMsg ) {
    uint16_t ManufacturerCode;
    uint8_t Reserved;
    uint8_t  IndustryCode;
    uint16_t ProprietaryID;
    uint8_t Command;

    if ( ParseN2kPGN126720( N2kMsg, ManufacturerCode, Reserved, IndustryCode, ProprietaryID, Command)) {
        ESP_LOGD(TAG,"ProprietaryFastPacket manu %d res %d indus %d propId %d cmd %d",
                 ManufacturerCode, Reserved, IndustryCode, ProprietaryID, Command );
    }
}

bool ParseN2kPGN65359(const tN2kMsg &N2kMsg, uint16_t& company, uint8_t& sid, uint16_t& headingTrue, uint16_t& headingMagnetic) {
    if (N2kMsg.PGN!=65359L) return false;

    /* see https://github.com/canboat/canboat/blob/master/analyzer/pgn.h */
    int Index=0;
    // company = ManufacturerCode(11) .. Reserved(2) .. IndustryCode(3)
    company = N2kMsg.Get2ByteUInt(Index);
    sid  =  N2kMsg.GetByte(Index);
    headingTrue = N2kMsg.Get2ByteUInt(Index);
    headingMagnetic = N2kMsg.Get2ByteUInt(Index);

    return true;
}

void HandleSeatalkPilotHeading( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint16_t  headingTrue;
    uint16_t headingMagnetic;

    if ( ParseN2kPGN65359( N2kMsg, company, sid, headingTrue, headingMagnetic)) {
        ESP_LOGD(TAG,"SeatalkPilotHeading company %d sid %d headTrue %d headMagnetic %d",
                 company, sid, headingTrue, headingMagnetic );
    }
}

bool ParseN2kPGN65360(const tN2kMsg &N2kMsg, uint16_t& company, uint8_t& sid, uint16_t& targetHeadingTrue, uint16_t& targetHeadingMagnetic) {
    if (N2kMsg.PGN!=65360L) return false;

    /* see https://github.com/canboat/canboat/blob/master/analyzer/pgn.h */
    int Index=0;
    // company = ManufacturerCode(11) .. Reserved(2) .. IndustryCode(3)
    company = N2kMsg.Get2ByteUInt(Index);
    sid  =  N2kMsg.GetByte(Index);
    targetHeadingTrue = N2kMsg.Get2ByteUInt(Index);
    targetHeadingMagnetic = N2kMsg.Get2ByteUInt(Index);

    return true;
}

void HandleSeatalkPilotLockedHeading( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint16_t  headingTrue;
    uint16_t headingMagnetic;

    if ( ParseN2kPGN65360( N2kMsg, company, sid, headingTrue, headingMagnetic)) {
        ESP_LOGD(TAG,"SeatalkPilotHeading company %d sid %d headTrue %d headMagnetic %d",
                 company, sid, headingTrue, headingMagnetic );
    }
}

bool ParseN2kPGN65379(const tN2kMsg &N2kMsg, uint16_t& company, uint8_t& pilotMode, uint8_t& subMode, uint8_t& pilotModeData) {
    if (N2kMsg.PGN!=65379L) return false;

    /* see https://github.com/canboat/canboat/blob/master/analyzer/pgn.h */
    int Index=0;
    // company = ManufacturerCode(11) .. Reserved(2) .. IndustryCode(3)
    company = N2kMsg.Get2ByteUInt(Index);
    // 0: standby, 64: auto
    pilotMode  =  N2kMsg.GetByte(Index);
    subMode  =  N2kMsg.GetByte(Index);
    pilotModeData  =  N2kMsg.GetByte(Index);
    // reserved

    return true;
}

void HandleSeatalkPilotMode( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t pilotMode;
    uint8_t subMode;
    uint8_t pilotModeData;

    if ( ParseN2kPGN65379( N2kMsg, company, pilotMode, subMode, pilotModeData)) {
        ESP_LOGD(TAG,"SeatalkPilotMode company %d mode %d sub %d data %d",
                 company, pilotMode, subMode, pilotModeData );
    }
}

bool ParseN2kPGN65288(const tN2kMsg &N2kMsg, uint16_t& company, uint8_t& sid, uint8_t& alarmStatus, uint8_t& alarmId, uint8_t& alarmGroup, uint8_t& alarmPriority) {
    if (N2kMsg.PGN!=65288L) return false;

    /* see https://github.com/canboat/canboat/blob/master/analyzer/pgn.h */
    int Index=0;
    // company = ManufacturerCode(11) .. Reserved(2) .. IndustryCode(3)
    company = N2kMsg.Get2ByteUInt(Index);
    sid  =  N2kMsg.GetByte(Index);
    // 0=Alarm condition not met
    // 1=Alarm condition met and not silenced
    // 2=Alarm condition met and silenced
    alarmStatus  =  N2kMsg.GetByte(Index);
    // 30=Pilot Drive Stopped
    // 32=Pilot Calibration Required
    // 51=Pilot No Wind Data
    // 80=Pilot Invalid Command
    alarmId  =  N2kMsg.GetByte(Index);
    alarmGroup  =  N2kMsg.GetByte(Index);
    alarmPriority  =  N2kMsg.GetByte(Index);

    return true;
}

void HandleSeatalkAlarm( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint8_t alarmStatus;
    uint8_t alarmId;
    uint8_t alarmGroup;
    uint8_t alarmPriority;

    if ( ParseN2kPGN65288( N2kMsg, company, sid, alarmStatus, alarmId,alarmGroup,alarmPriority)) {
        ESP_LOGD(TAG,"SeatalkAlarm company %d sid %d status %d id %d group %d prio %d",
                 company, sid, alarmStatus, alarmId ,alarmGroup,alarmPriority);
    }
}

/*
 {"Seatalk: Silence Alarm",
     65361,
     PACKET_COMPLETE,
     PACKET_SINGLE,
     {COMPANY(1851),
      LOOKUP_FIELD("Alarm ID", BYTES(1), SEATALK_ALARM_ID),
      LOOKUP_FIELD("Alarm Group", BYTES(1), SEATALK_ALARM_GROUP),
      RESERVED_FIELD(32),
      END_OF_FIELDS}}

    61184:  Manufacturer Proprietary single-frame addressed

   {"Seatalk: Wireless Keypad Light Control",
     61184,
     PACKET_INCOMPLETE,
     PACKET_SINGLE,
     {COMPANY(1851),
      MATCH_FIELD("Proprietary ID", BYTES(1), 1, "Wireless Keypad Light Control"),
      UINT8_FIELD("Variant"),
      UINT8_FIELD("Wireless Setting"),
      UINT8_FIELD("Wired Setting"),
      RESERVED_FIELD(BYTES(2)),
      END_OF_FIELDS}}

    {"Seatalk: Wireless Keypad Control",
     61184,
     PACKET_INCOMPLETE,
     PACKET_SINGLE,
     {COMPANY(1851),
      UINT8_FIELD("PID"),
      UINT8_FIELD("Variant"),
      UINT8_FIELD("Beep Control"),
      RESERVED_FIELD(BYTES(3)),
      END_OF_FIELDS}}
*/


//*****************************************************************************
void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg ) {
    ESP_LOGD(TAG,"Sending PGN %05lx %06ld", N2kMsg.PGN, N2kMsg.PGN);
    switch ( N2kMsg.PGN ) {
        case 61184:
//            HandleSeatalkKeypadControl(N2kMsg);
            break;
        case 65288L:
            HandleSeatalkAlarm(N2kMsg);
            break;
        case 65359L:
            HandleSeatalkPilotHeading( N2kMsg );
            break;
        case 65360L:
            HandleSeatalkPilotLockedHeading( N2kMsg );
            break;
        case 65361L:
//            HandleSeatalkSilenceAlarm( N2kMsg );
            break;
        case 65379L:
            HandleSeatalkPilotMode( N2kMsg );
            break;
        case 126208L:
            // NMEA - Request group function: The receiver shall respond by sending the requested PGN, at the desired transmission interval.
            break;
        case 126720L:
            HandleProprietaryFastPacket( N2kMsg );
            break;
        case 127245L:
            HandleRudder( N2kMsg );
            break;
        case 127250L:
            HandleHeading( N2kMsg );
            break;
        case 127257L:
            HandleAttitude( N2kMsg );
            break;
        case 128259L:
            HandleBoatSpeed( N2kMsg );
            break;
        case 128275L:
            HandleLog( N2kMsg );
            break;
        case 128267L:
            HandleDepth( N2kMsg );
            break;
        case 129025L:
            HandlePosition( N2kMsg );
            break;
        case 129026L:
            HandleCOG_SOG( N2kMsg );
            break;
        case 129029L:
            HandleGNSS( N2kMsg );
            break;
        case 130306L:
            HandleWind( N2kMsg );
            break;
        case 130310L:
            HandleWaterTemp( N2kMsg );
            break;
        case 59904L: // ISO request
        case 60928L: // Address claim
        case 65384L: // unknown and dropped
            break;
        default:
            ESP_LOGW(TAG,"Dropped PGN %05lx %06ld", N2kMsg.PGN, N2kMsg.PGN);
            break;
    }
    ESP_LOGD(TAG,"Sent");
}
