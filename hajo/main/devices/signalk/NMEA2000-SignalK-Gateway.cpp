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
#include "NMEA2000.h"
#include "N2kMessages.h"
#include "N2kMsg.h"
#include "N2kTypes.h"
#include "n2k/N2kRaymarine.h"

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

const char* lookupName( const char *names[], size_t sizeInBytes, uint8_t value ) {
    if ( value >= sizeInBytes / sizeof(const char*) || names[value] == nullptr) {
        return "bad_index";
    }
    return names[value];
}

const char* lookupName( const LookupEntry names[], size_t sizeInBytes, uint8_t value ) {
    for ( uint i = 0; i < sizeInBytes / sizeof(LookupEntry); i++) {
        if ( names[i].value == value) {
            return names[i].name;
        }
    }
    return "bad_index";
}

//*****************************************************************************
void HandleHeading( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kHeadingReference reference;
    double Deviation = 0;
    double Variation;
    double Heading;

    if ( ParseN2kHeading( N2kMsg, SID, Heading, Deviation, Variation, reference )) {
        //ESP_LOGI( TAG, "PGN Heading head %f dev %f var %f ref %d", Heading, Deviation, Variation, reference );
        double value;
        if ( reference == N2khr_magnetic) {
            if ( Deviation == -1000000000.000000 ) {
                Deviation = 0.0;
            }
            if ( Variation == -1000000000.000000 ) {
                Variation = 0.0;
            }
            value = Heading + Variation + Deviation;
        } else {
            value = Heading;
        }
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.headingTrue", value );
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandleBoatSpeed( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    double WaterReferenced;
    double GroundReferenced;
    tN2kSpeedWaterReferenceType SWRT;

    if ( ParseN2kBoatSpeed( N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT )) {
        /*
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
         -1000000000.000000
        deltaSet.addValue( "navigation.speedThroughWater", WaterReferenced );
        deltaSet.addValue( "navigation.speedOverGround", GroundReferenced );
        deltaSet.send( sigK );
         */
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
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "environment.depth.belowKeel", WaterDepth );
        deltaSet.addValue( "environment.depth.belowTransducer", WaterDepth );
        deltaSet.addValue( "environment.depth.belowSurface", WaterDepth);
        // TODO depth/transducerToKeel
        // TODO depth/surfaceToTransducer
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandlePosition( const tN2kMsg &N2kMsg ) {
    double Latitude;
    double Longitude;
    char buf[100];

    if ( ParseN2kPGN129025( N2kMsg, Latitude, Longitude )) {
        snprintf( buf, sizeof( buf ), R"({"altitude":%f,"latitude":%f,"longitude":%f})", 0.0, Latitude, Longitude );
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.position", buf );
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandleCOG_SOG( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kHeadingReference ref;
    double COG;
    double SOG;

    if ( ParseN2kPGN129026( N2kMsg, SID, ref, COG, SOG )) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.courseOverGroundTrue", COG );
        deltaSet.addValue( "navigation.speedOverGround", SOG );
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandleWind( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    tN2kWindReference WindReference;

    double WindAngle, WindSpeed;

    if ( ParseN2kWindSpeed( N2kMsg, SID, WindSpeed, WindAngle, WindReference )) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        if ( WindReference == N2kWind_Apparent ) {
            deltaSet.addValue( "environment.wind.angleApparent", WindAngle );
            deltaSet.addValue( "environment.wind.speedApparent", WindSpeed );
        } else if ( WindReference == N2kWind_True_boat ) {
            deltaSet.addValue( "environment.wind.angleTrueGround", WindAngle );
            deltaSet.addValue( "environment.wind.speedTrue", WindSpeed );
        } else if ( WindReference == N2kWind_True_water ) {
            deltaSet.addValue( "environment.wind.angleTrueWater", WindAngle );
            deltaSet.addValue( "environment.wind.speedTrue", WindSpeed );
        }
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandleLog( const tN2kMsg &N2kMsg ) {

    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    uint32_t Log;
    uint32_t TripLog;

    if ( ParseN2kDistanceLog( N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog )) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.trip.log", (int) TripLog );
        deltaSet.addValue( "navigation.log", (int) Log );
        deltaSet.send( sigK );
    }
}


//*****************************************************************************
void HandleWaterTemp( const tN2kMsg &N2kMsg ) {

    unsigned char SID;
    double OutsideAmbientAirTemperature;
    double AtmosphericPressure;
    double WaterTemperature;

    if ( ParseN2kPGN130310( N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure )) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        // deltaSet.addDeltaValue("environment.outside.temperature", OutsideAmbientAirTemperature);
        // deltaSet.addDeltaValue("environment.outside.pressure", AtmosphericPressure);
        deltaSet.addValue( "environment.water.temperature", WaterTemperature );
        deltaSet.send( sigK );
    }
}

void HandleBatteryDetailedStatus( const tN2kMsg &N2kMsg ) {
    unsigned char SID;
    unsigned char DCInstance;
    tN2kDCType DCType;
    uint8_t StateOfCharge;
    uint8_t StateOfHealth;
    double TimeRemaining;
    double RippleVoltage;
    double Capacity;

    if ( ParseN2kPGN127506( N2kMsg, SID, DCInstance,DCType,StateOfCharge,StateOfHealth,TimeRemaining,RippleVoltage,Capacity)) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        char path[64];
        snprintf( path, sizeof(path), "electrical.batteries.%d.rippleVoltage",DCInstance+1);
        deltaSet.addValue( path, RippleVoltage);
        deltaSet.send( sigK );
    }
}

void HandleBatteryStatus( const tN2kMsg &N2kMsg ) {
    unsigned char BatteryInstance;
    double BatteryVoltage;
    double BatteryCurrent;
    double BatteryTemperature;
    unsigned char SID;

    if ( ParseN2kPGN127508( N2kMsg, BatteryInstance,BatteryVoltage,BatteryCurrent,BatteryTemperature,SID)) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        char path[64];
        snprintf( path, sizeof(path), "electrical.batteries.%d.voltage", BatteryInstance+1);
        deltaSet.addValue( path, BatteryVoltage);
        deltaSet.send( sigK );
    }
}

void HandleFluidLevel( const tN2kMsg &N2kMsg ) {
    unsigned char Instance;
    tN2kFluidType FluidType;
    double Level;
    double Capacity;

    if ( ParseN2kPGN127505( N2kMsg, Instance,FluidType,Level,Capacity)) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        char path[64];
        const char* typeName;
        switch ( FluidType ) {
            case N2kft_Fuel:
                typeName ="fuel";
                break;
            case N2kft_Water:
                typeName ="freshWater";
                break;
            case N2kft_GrayWater:
                typeName ="wasteWater";
                break;
            case N2kft_LiveWell:
                typeName ="liveWell";
                break;
            case N2kft_Oil:
                typeName ="lubrication";
                break;
            case N2kft_BlackWater:
                typeName ="blackWater";
                break;
            case N2kft_FuelGasoline:
                typeName ="fuel";
                break;
            default:
                typeName ="other";
                break;
        }
        snprintf( path, sizeof(path), "tanks.%s.%d.currentLevel", typeName,Instance+1);
        // capacity, currentVolume
        deltaSet.addValue( path,Level);
        deltaSet.send( sigK );
    }
}

//*****************************************************************************
void HandleRudder( const tN2kMsg &N2kMsg ) {

    double RudderPosition;
    unsigned char Instance;
    tN2kRudderDirectionOrder RudderDirectionOrder;
    double AngleOrder;

    if ( ParseN2kRudder( N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder )) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "steering.rudderAngle", RudderPosition );
        deltaSet.send( sigK );
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

        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.gnss.type", GNSSType );
        deltaSet.addValue( "navigation.gnss.horizontalDilution", HDOP );
        deltaSet.addValue( "navigation.gnss.positionDilution", PDOP );

//        deltaSet.send( sigK );

        deltaSet.addValue( "navigation.gnss.satellites", nSatellites );
        deltaSet.addValue( "navigation.gnss.geoidalSeparation", GeoidalSeparation );
        deltaSet.addValue( "navigation.gnss.differentialAge", AgeOfCorrection );

//        deltaSet.send( sigK );

        deltaSet.addValue( "navigation.gnss.differentialReference", ReferenceStationID );
        snprintf( buf, sizeof( buf ), R"({"altitude":%f,"latitude":%f,"longitude":%f})", Altitude, Latitude,
                  Longitude );
        deltaSet.addValue( "navigation.position", buf );

        deltaSet.send( sigK );
    }
}


void HandleAttitude( const tN2kMsg &N2kMsg ) {
    unsigned char sid ;
    double yaw;
    double pitch;
    double roll ;

    if ( ParseN2kAttitude( N2kMsg, sid, yaw, pitch, roll)) {
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        deltaSet.addValue( "navigation.attitude.pitch", pitch );
        deltaSet.addValue( "navigation.attitude.roll", roll );
        deltaSet.addValue( "navigation.attitude.yaw", yaw );
        deltaSet.send( sigK );
    }
}

bool HandleSeaTalkFastPacket33264(const tN2kMsg &N2kMsg, int &Index) {
    uint8_t command = N2kMsg.GetByte(Index);
    switch (command) {
        case 134: {
            uint8_t device = N2kMsg.GetByte(Index);
            uint8_t key = N2kMsg.GetByte(Index);
            uint8_t keyInverted = N2kMsg.GetByte(Index);
            // -1: 80, +1: 81, -10: 82, +10: 83, released: 84
            ESP_LOGI( TAG, R"(SeaTalk keystroke device %d key %02x keyInverted %02x)",
                      device, key, keyInverted );
            break;
        }
        case 144: {
            N2kMsg.GetByte(Index);
            uint8_t device = N2kMsg.GetByte(Index);
            ESP_LOGI( TAG, R"(SeaTalk device identification device %d "%s" dataLen %d)",
                      device,
                      lookupName(SeaTalkDeviceId,sizeof (SeaTalkDeviceId), device),
                      N2kMsg.DataLen-Index );
            break;
        }
        case 131:
        case 132:
        case 150:
        case 154:
        case 156:
        case 174:
            // These come from autopilot very quickly
            break;
        default:
            ESP_LOGI(TAG, R"(SeaTalk propId %d command %d dataLen %d)",
                     33264, command, N2kMsg.DataLen-Index);
            break;
    }
    return true;
}

bool HandleSeaTalkFastPacket3212(const tN2kMsg &N2kMsg, int Index) {
    uint8_t group = N2kMsg.GetByte(Index);
    if ( group == 255) {
        // 255 is probably "not yet configured", use "none" instead
        group = 0;
    }
    //uint8_t unknown1 =
    N2kMsg.GetByte(Index);
    uint8_t command = N2kMsg.GetByte(Index);
    uint8_t value = N2kMsg.GetByte(Index);
    //uint8_t unknown2 =
    N2kMsg.GetByte(Index);
    switch ( command) {
        case 0:
            ESP_LOGI(TAG, R"(SeaTalk display brightness group %d "%s" brightness %d%%)",
                     group,
                     lookupName(SeaTalkNetworkGroup, sizeof(SeaTalkNetworkGroup), group),
                     value);
            break;
        case 1:
            ESP_LOGI(TAG, R"(SeaTalk display color group %d "%s" color %s)",
                     group,
                     lookupName(SeaTalkNetworkGroup, sizeof(SeaTalkNetworkGroup), group),
                     lookupName(SeaTalkDisplayColor, sizeof(SeaTalkDisplayColor), group));
            break;
        default:
            ESP_LOGI(TAG, "SeaTalk propId %d command %d value %d dataLen %d",
                     3212, command, value, N2kMsg.DataLen-Index);
            break;
    }
    return true;
}

void HandleProprietaryFastPacket(const tN2kMsg &N2kMsg ) {
    uint16_t ManufacturerCode;
    uint8_t Reserved;
    uint8_t  IndustryCode;
    uint16_t ProprietaryID;
    int Index = 0;

    if ( ParseN2kPGN126720( N2kMsg, Index, ManufacturerCode, Reserved, IndustryCode, ProprietaryID)) {
        /*
         * manu 1851 res 3 indus 4 propId 33264 cmd 131/132/154/150/174/156/144
         */
        if ( ManufacturerCode == 1851 ) {
            // SeaTalk
            bool processed = false;
            if ( ProprietaryID == 33264 ) {
                processed = HandleSeaTalkFastPacket33264(N2kMsg, Index);
            } if ( ProprietaryID == 3212 ) {
                processed = HandleSeaTalkFastPacket3212(N2kMsg, Index);
            }
            if ( !processed) {
                ESP_LOGI(TAG, "ProprietaryFastPacket RayMarine propId %d dataLen %d",
                         ProprietaryID, N2kMsg.DataLen);
            }
        } else {
            ESP_LOGI( TAG, "ProprietaryFastPacket manu %d res %d indus %d propId %d dataLen %d",
                      ManufacturerCode, Reserved, IndustryCode, ProprietaryID, N2kMsg.DataLen );
        }
    }
}

void HandleSeatalkPilotHeading( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint16_t  headingTrue;
    uint16_t headingMagnetic;

    if ( ParseN2kPGN65359( N2kMsg, company, sid, headingTrue, headingMagnetic)) {
        // company 40763 sid 255 headTrue 65535 headMagnetic 13884
        ESP_LOGD(TAG,"SeaTalk PilotHeading sid %d headTrue %d headMagnetic %d",
                 sid, headingTrue, headingMagnetic );
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        if ( headingTrue == 65535 ) {
            deltaSet.addValue( "steering.autopilot.target.headingTrue", "invalid");
        } else {
            deltaSet.addValue( "steering.autopilot.target.headingTrue", headingTrue / 10000.0 );
        }
        deltaSet.addValue( "steering.autopilot.target.headingMagnetic", headingMagnetic / 10000.0 );
        deltaSet.send( sigK );
    }
}

void HandleSeatalkPilotLockedHeading( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint16_t  headingTrue;
    uint16_t headingMagnetic;

    if ( ParseN2kPGN65360( N2kMsg, company, sid, headingTrue, headingMagnetic)) {
        ESP_LOGI(TAG,"SeaTalk PilotLockedHeading sid %d headTrue %d headMagnetic %d",
                 sid, headingTrue, headingMagnetic );
    }
}

void HandleSeatalkPilotMode( const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint16_t pilotMode;
    uint8_t subMode;
    uint8_t pilotModeData;

    if ( ParseN2kPGN65379( N2kMsg, company, pilotMode, subMode, pilotModeData)) {
        ESP_LOGI(TAG, R"(SeaTalk PilotMode mode %d "%s" subMode %d data %d)",
                 pilotMode,
                 lookupName(SeaTalkPilotMode16, sizeof(SeaTalkPilotMode16), pilotMode),
                 subMode,
                 pilotModeData );
        DeltaSet deltaSet(N2kMsg.Source, N2kMsg.PGN);
        const char *modeValue = pilotMode == 64 ? "auto" : pilotMode == 256 ? "wind": pilotMode == 384 ? "route" : "standby";
        deltaSet.addValue( "steering.autopilot.mode", modeValue );
        deltaSet.send( sigK );
    }
}

void HandleSeaTalkAlarm(const tN2kMsg &N2kMsg ) {
    uint16_t company;
    uint8_t sid;
    uint8_t alarmStatus;
    uint8_t alarmId;
    uint8_t alarmGroup;
    uint8_t alarmPriority;

    if ( ParseN2kPGN65288( N2kMsg, company, sid, alarmStatus, alarmId,alarmGroup,alarmPriority)) {
        ESP_LOGI( TAG, R"(SeaTalk alarm sid %d status %d "%s" id %d "%s" group %d "%s" priority %d)",
                  sid,
                  alarmStatus, lookupName( SeaTalkAlarmStatus, sizeof(SeaTalkAlarmStatus), alarmStatus ),
                  alarmId , lookupName( SeaTalkAlarmId, sizeof(SeaTalkAlarmId), alarmId ),
                  alarmGroup, lookupName( SeaTalkAlarmGroup, sizeof(SeaTalkAlarmGroup), alarmGroup ),
                  alarmPriority);
    }
}

void HandleTimeAndDate( const tN2kMsg &N2kMsg ) {
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    int16_t LocalOffset;

    if (ParseN2kPGN129033( N2kMsg,DaysSince1970, SecondsSinceMidnight,LocalOffset) ) {

    }
}

void HandleSeatalkSilenceAlarm(const tN2kMsg &N2kMsg) {
    uint8_t alarmId;
    uint8_t alarmGroup;

    if ( ParseN2kPGN65361( N2kMsg, alarmId,alarmGroup)) {
        ESP_LOGI( TAG, R"(SeaTalk silence alarm id %d "%s" group %d "%s")",
                  alarmId , lookupName( SeaTalkAlarmId, sizeof(SeaTalkAlarmId), alarmId ),
                  alarmGroup, lookupName( SeaTalkAlarmGroup, sizeof(SeaTalkAlarmGroup), alarmGroup ));
    }
}

void HandleSeatalkKeypadControl(const tN2kMsg &N2kMsg) {
    uint8_t proprietaryID;
    uint8_t variant;
     uint8_t wirelessSetting;
            uint8_t wiredSetting;
            uint8_t beepControl;

    if ( ParseN2kPGN61184( N2kMsg, proprietaryID, variant,wirelessSetting,wiredSetting,beepControl)) {
        ESP_LOGI( TAG, R"(SeaTalk keypad control proprietaryID %d variant %d wirelessSetting %d wiredSetting %d beepControl %d)",
                  proprietaryID,variant,wirelessSetting,wiredSetting,beepControl);
    }
}

void HandleProductInformation( const tN2kMsg &N2kMsg ) {
    tNMEA2000::tProductInformation productInfo = {};

    if (        ParseN2kPGN126996(N2kMsg,productInfo.N2kVersion,productInfo.ProductCode,
                                  sizeof(productInfo.N2kModelID),productInfo.N2kModelID,
                                  sizeof(productInfo.N2kSwCode),productInfo.N2kSwCode,
                                  sizeof(productInfo.N2kModelVersion),productInfo.N2kModelVersion,
                                  sizeof(productInfo.N2kModelSerialCode),productInfo.N2kModelSerialCode,
                                  productInfo.CertificationLevel,productInfo.LoadEquivalency) ) {
        ESP_LOGI(TAG,"product info modelId %s version %s serial %s",
                 productInfo.N2kModelID,
                 productInfo.N2kModelVersion,
                 productInfo.N2kModelSerialCode );
    }
}

//*****************************************************************************

// see https://signalk.org/specification/1.5.0/doc/vesselsBranch.html

void sendN2KMessageToSignalK(const tN2kMsg &N2kMsg ) {
    // set CONFIG_NMEA2000_MSG_DEBUG=y in sdkconfig to see low level messages
    ESP_LOGD(TAG,"Sending PGN %05lx %06ld", N2kMsg.PGN, N2kMsg.PGN);
    switch ( N2kMsg.PGN ) {
        case 61184L:
            HandleSeatalkKeypadControl(N2kMsg);
            break;
        case 65288L:
            // TODO send to SignalK
            HandleSeaTalkAlarm(N2kMsg);
            break;
        case 65359L:
            HandleSeatalkPilotHeading( N2kMsg );
            break;
        case 65360L:
            // TODO send to SignalK
            HandleSeatalkPilotLockedHeading( N2kMsg );
            break;
        case 65361L:
            HandleSeatalkSilenceAlarm( N2kMsg );
            break;
        case 65379L:
            HandleSeatalkPilotMode( N2kMsg );
            break;
        case 126208L:
            // NMEA - Request group function: The receiver shall respond by sending the requested PGN, at the desired transmission interval.
            break;
        case 126720L:
            // TODO send to SignalK
            HandleProprietaryFastPacket( N2kMsg );
            break;
        case 126996L:
            HandleProductInformation( N2kMsg );
            break;
        case 127505L:
            HandleFluidLevel(N2kMsg);
            break;
        case 127506L:
            // not interested in this one
            //HandleBatteryDetailedStatus( N2kMsg );
            break;
        case 127508L:
            HandleBatteryStatus( N2kMsg );
            break;
        case 127513L:
            // not interested in this one
            //HandleBatteryConfiguration( N2kMsg );
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
        case 129033L:
            HandleTimeAndDate( N2kMsg );
            break;
        case 130306L:
            HandleWind( N2kMsg );
            break;
        case 130310L:
            HandleWaterTemp( N2kMsg );
            break;
        case 129540L: // ISO request
        case 130312L:
            // TODO implement these
            break;
        case 59904L: // ISO request
        case 60928L: // Address claim
        case 65384L: // unknown and dropped
        case 126993L: // HeartBeat
            // not interested in there
            break;
        case 65362L:
        case 65370L:
        case 65381L:
            // not able to find these
            break;
        default:
            ESP_LOGW(TAG,"Unhandled PGN %05lx %06ld", N2kMsg.PGN, N2kMsg.PGN);
            break;
    }
    ESP_LOGD(TAG,"Sent");
}

/*
 * Seatalk PilotMode mode 64 "bad_index" sub 0 data 2
 * Seatalk PilotHeading sid 255 headTrue 65535 headMagnetic 15937
 * SeaTalk silence alarm id 59 "Pilot Lost Waypoint Data" group 159 "bad_index"
 */

/*

environment/wind/speedTrue
environment/wind/speedOverGround
environment/wind/speedApparent
environment/wind/angleApparent
environment/wind/angleTrueGround
environment/wind/angleTrueWater
navigation/speedOverGround
navigation/speedThroughWater
navigation/courseOverGroundMagnetic

129540L: GNSS Sats in View, pri=6, period=1000
130312L: Temperature, pri=5, period=2000

129029:
navigation.gnss.differentialAge -1000000000.000000"
navigation.gnss.geoidalSeparation -1000000000.000000"
navigation.gnss.positionDilution -1000000000.000000"

128259:
navigation.speedOverGround -1000000000.000000"
 */