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

#include "N2kMessages.h"
#include "N2kMsg.h"
#include "N2kTypes.h"

#include "devices/signalk/EspSigK.h"        // For SignalK handling

extern EspSigK sigK;

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

        sigK.addDeltaValue( "navigation.gnss.type", GNSSType );
        sigK.addDeltaValue( "navigation.gnss.horizontalDilution", HDOP );
        sigK.addDeltaValue( "navigation.gnss.positionDilution", PDOP );

        sigK.sendDelta();

        sigK.addDeltaValue( "navigation.gnss.satellites", nSatellites );
        sigK.addDeltaValue( "navigation.gnss.geoidalSeparation", GeoidalSeparation );
        sigK.addDeltaValue( "navigation.gnss.differentialAge", AgeOfCorrection );

        sigK.sendDelta();

        sigK.addDeltaValue( "navigation.gnss.differentialReference", ReferenceStationID );
        snprintf( buf, sizeof( buf ), R"({"altitude":%f,"latitude":%f,"longitude":%f})", Altitude, Latitude,
                  Longitude );
        sigK.addDeltaValue( "navigation.position", buf );

        sigK.sendDelta();
    }
}


//*****************************************************************************
void sendN2KMessageToSignalK( const tN2kMsg &N2kMsg ) {

    switch ( N2kMsg.PGN ) {
        case 127250L:
            HandleHeading( N2kMsg );
        case 128259L:
            HandleBoatSpeed( N2kMsg );
        case 128267L:
            HandleDepth( N2kMsg );
        case 129025L:
            HandlePosition( N2kMsg );
        case 129026L:
            HandleCOG_SOG( N2kMsg );
        case 129029L:
            HandleGNSS( N2kMsg );
        case 130306L:
            HandleWind( N2kMsg );
        case 128275L:
            HandleLog( N2kMsg );
        case 130310L:
            HandleWaterTemp( N2kMsg );
        case 127245L:
            HandleRudder( N2kMsg );
    }
}
