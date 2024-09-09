#ifndef HAJO_N2K_STRUCT_PARSER_H
#define HAJO_N2K_STRUCT_PARSER_H

#include "N2kMsg.h"
#include "N2kTypes.h"

/*
 * Documents:
 *
 * https://copperhilltech.com/content/NMEA2K_Network_Design_v2.pdf
 * https://canboat.github.io/canboat/canboat.xml
 *
 */

/*
 * single/fast packet ranges
 *
 * PDU1 (addressed) single-frame PGN range 0E800 to 0xEEFF (59392 - 61183)
 * PDU1 (addressed) single-frame PGN range 0EF00 to 0xEFFF (61184 - 61439)
 * PDU2 non-addressed single-frame PGN range 0xF000 to 0xFEFF (61440 - 65279)
 * PDU2 (non-addressed) single-frame PGN range 0xFF00 to 0xFFFF (65280 - 65535)
 * PDU1 (addressed) fast-packet PGN range 0x10000 to 0x1EE00 (65536 - 126464)
 * PDU1 (addressed) fast-packet PGN range 0x1EF00 to 0x1EFFF (126720 - 126975)
 * PDU2 (non-addressed) mixed single/fast packet PGN range 0x1F000 to 0x1FEFF (126976 - 130815)
 * PDU2 (non-addressed) fast packet PGN range 0x1FF00 to 0x1FFFF (130816 - 131071)
 */

/*
 * PNGs
 *
 * - 0x0EE00: PGN 60928 - ISO Address Claim
 * - 0x1F007: PGN 126983 - Alert
 * - 0x1F010: PGN 126992 - System Time
 * - 0x1F011: PGN 126993 - Heartbeat
 * - 0x1F014: PGN 126996 - Product Information
 * - 0x1F016: PGN 126998 - Configuration Information
 * - 0x1F105: PGN 127237 - Heading/Track control
 * - 0x1F10D: PGN 127245 - Rudder
 * - 0x1F112: PGN 127250 - Vessel Heading
 * - 0x1F113: PGN 127251 - Rate of Turn
 * - 0x1F114: PGN 127252 - Heave
 * - 0x1F211: PGN 127505 - Fluid Level
 * - 0x1F214: PGN 127508 - Battery Status
 * - 0x1F503: PGN 128259 - Speed
 * - 0x1F50B: PGN 128267 - Water Depth
 * - 0x1F513: PGN 128275 - Distance Log
 * - 0x1F805: PGN 129029 - GNSS Position Data
 * - 0x1F809: PGN 129033 - Time & Date
 * - 0x1FD02: PGN 130306 - Wind Data
 */

/*
 * const tN2kMsg &N2kMsg, unsigned char &SID, uint16_t &DaysSince1970, double &SecondsSinceMidnight,
                     double &Latitude, double &Longitude, double &Altitude,
                     tN2kGNSStype &GNSStype, tN2kGNSSmethod &GNSSmethod,
                     uint8_t &nSatellites, double &HDOP, double &PDOP, double &GeoidalSeparation,
                     uint8_t &nReferenceStations, tN2kGNSStype &ReferenceStationType, uint16_t &ReferenceSationID,
                     double &AgeOfCorrection
 */

#define N2K_PGN_FLUID_LEVEL 0x1F211
#define N2K_PGN_FLUID_LEVEL_INTERVAL_MS 2500

typedef struct {
    unsigned char instance;
    tN2kFluidType fluidType;
    double level;
    double capacity;
} N2kFluidLevelData;

extern bool ParseN2kFluidLevel( const tN2kMsg &N2kMsg, N2kFluidLevelData &data );

#define N2K_PGN_DC_DETAILED_STATUS 0x1F212  // 127506
#define N2K_PGN_DC_DETAILED_STATUS_INTERVAL_MS 1500

typedef struct {
    unsigned char sid;
    unsigned char instance;
    tN2kDCType dcType;
    unsigned char stateOfCharge;
    unsigned char stateOfHealth;
    double timeRemaining;
    double rippleVoltage;
    double capacity;
} ParseN2kDCStatusData;

extern bool ParseN2kDCStatus( const tN2kMsg &N2kMsg, ParseN2kDCStatusData &data );

#define N2K_PGN_BATTERY_STATUS 0x1F214 // 127508
#define N2K_PGN_BATTERY_STATUS_INTERVAL_MS 1500

typedef struct {
    unsigned char instance;
    double voltage;
    double current;
    double temperature;
    unsigned char sid;
} N2kDCBatStatusData;

extern bool ParseN2kDCBatStatus( const tN2kMsg &N2kMsg, N2kDCBatStatusData &data );

#define N2K_PGN_BATTERY_CONFIGURATION 0x1F219 // 127513
#define N2K_PGN_BATTERY_CONFIGURATION_INTERVAL_MS 5000

typedef struct {
    unsigned char instance;
    tN2kBatType batType;
    tN2kBatEqSupport supportsEqual;
    tN2kBatNomVolt batNominalVoltage;
    tN2kBatChem batChemistry;
    double batCapacity;
    int8_t batTemperatureCoefficient;
    double peukertExponent;
    int8_t chargeEfficiencyFactor;
} N2kBatConfData;

extern bool ParseN2kBatConf( const tN2kMsg &N2kMsg, N2kBatConfData &data );

#define N2K_PGN_GNSS_POSITION_DATA 0x1F805 // 129029
#define N2K_PGN_GNSS_POSITION_DATA_INTERVAL_MS 1000

typedef struct {
    unsigned char sid;
    uint16_t daysSince1970;
    double secondsSinceMidnight;
    double latitude;
    double longitude;
    double altitude;
    tN2kGNSStype gnssType;
    tN2kGNSSmethod gnssMethod;
    unsigned char satellites;
    double hdop;
    double pdop;
    double geoidalSeparation;
    unsigned char referenceStations;
    tN2kGNSStype referenceStationType;
    uint16_t referenceStationID;
    double ageOfCorrection;
} N2kGNSSData;

extern bool ParseN2kGNSS( const tN2kMsg &N2kMsg, N2kGNSSData &data );

#define N2K_PGN_LOCAL_OFFSET 0x1F809 // 129029
#define N2K_PGN_LOCAL_OFFSET_INTERVAL_MS 1000

typedef struct {
    uint16_t daysSince1970;
    double secondsSinceMidnight;
    int16_t localOffset;
} N2kLocalOffsetData;

extern bool ParseN2kLocalOffset( const tN2kMsg &N2kMsg, N2kLocalOffsetData &data );

#define N2K_PGN_PROPRIETARY_FAST_PACKET   0x1ef00 // 126720

typedef struct {
    unsigned int manufacturerCode: 11;
    unsigned int reserved: 2;
    unsigned int industryCode: 3;
    unsigned int proprietaryID: 18;
    unsigned int command: 8;
} N2kProprietaryFastPacketFrame;

#define N2K_PGN_RUDDER 0x1F10D // 127245
#define N2K_PGN_RUDDER_INTERVAL_MS 100

typedef struct {
    double rudderPosition;
    unsigned char instance;
    tN2kRudderDirectionOrder rudderDirectionOrder;
    double angleOrder;
} N2kRudderData;

extern bool ParseN2kRudder( const tN2kMsg &N2kMsg, N2kRudderData &data );

#define N2K_PGN_ATTITUDE 0x1F119 // 127257
#define N2K_PGN_ATTITUDE_INTERVAL_MS 1000

typedef struct {
    unsigned char instance;
    double yaw;
    double pitch;
    double roll;
} N2kAttitudeData;

extern bool ParseN2kAttitude( const tN2kMsg &N2kMsg, N2kAttitudeData &data );

#define N2K_PGN_HEADING 0x1F112 // 127250
#define N2K_PGN_HEADING_INTERVAL_MS 100

#endif //HAJO_N2K_STRUCT_PARSER_H
