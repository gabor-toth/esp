#include "N2kRaymarine.h"

#define RaymarineProprietary 0x037b // Raymarine 1851 + reserved + industry code=marine

/*
 * see
 * https://github.com/cape-io/canboat-web/blob/master/index.json
 * https://github.com/canboat/canboat/blob/master/analyzer/pgn.h
 * https://canboat.github.io/canboat/canboat.html
 */

/*
126208: // if length is 28: command to set to standby or auto
// if length is 25: command to set to heading
126720: // message from EV1 (204) indicating auto or standby state
65359: // heading all the time
65360: // Autopilot heading. From pilot. Transmitted only when pilot is auto.
 */

bool ParseN2kPGN65288( const tN2kMsg &N2kMsg, uint16_t &company, uint8_t &sid, uint8_t &alarmStatus, uint8_t &alarmId,
                       uint8_t &alarmGroup, uint8_t &alarmPriority ) {
    if ( N2kMsg.PGN != 65288L ) { return false; }

    int Index = 0;
    company = N2kMsg.Get2ByteUInt( Index );
    sid = N2kMsg.GetByte( Index );
    alarmStatus = N2kMsg.GetByte( Index );
    alarmId = N2kMsg.GetByte( Index );
    alarmGroup = N2kMsg.GetByte( Index );
    alarmPriority = N2kMsg.GetByte( Index );

    return true;
}

bool ParseN2kPGN65361( const tN2kMsg &N2kMsg, uint8_t &alarmId, uint8_t &alarmGroup ) {
    if ( N2kMsg.PGN != 65361L ) { return false; }

    int Index = 0;
    alarmId = N2kMsg.GetByte( Index );
    alarmGroup = N2kMsg.GetByte( Index );

    return true;
}

bool ParseN2kPGN61184( const tN2kMsg &N2kMsg, uint8_t &proprietaryID, uint8_t &variant, uint8_t &wirelessSetting,
                       uint8_t &wiredSetting, uint8_t &beepControl ) {
    if ( N2kMsg.PGN != 61184L ) { return false; }

    int Index = 0;
    proprietaryID = N2kMsg.GetByte( Index );
    variant = N2kMsg.GetByte( Index );
    if ( proprietaryID == 1 ) {
        wirelessSetting = N2kMsg.GetByte( Index );
        wiredSetting = N2kMsg.GetByte( Index );
        beepControl = 0;
    } else {
        wirelessSetting = 0;
        wiredSetting = 0;
        beepControl = N2kMsg.GetByte( Index );
    }

    return true;
}

bool ParseN2kPGN65360( const tN2kMsg &N2kMsg, uint16_t &company, uint8_t &sid, double &targetHeadingTrue,
                       double &targetHeadingMagnetic ) {
    if ( N2kMsg.PGN != 65360L ) { return false; }

    int Index = 0;
    company = N2kMsg.Get2ByteUInt( Index );
    sid = N2kMsg.GetByte( Index );
    targetHeadingTrue = N2kMsg.Get2ByteUDouble( 0.0001, Index );
    targetHeadingMagnetic = N2kMsg.Get2ByteUDouble( 0.0001, Index );

    return true;
}

bool ParseN2kPGN65359( const tN2kMsg &N2kMsg, uint8_t &sid, double &headingTrue, double &headingMagnetic ) {
    if ( N2kMsg.PGN != 65359L ) { return false; }

    int Index = 0;
    N2kMsg.Get2ByteUInt( Index );
    sid = N2kMsg.GetByte( Index );
    headingTrue = N2kMsg.Get2ByteUDouble( 0.0001, Index );
    headingMagnetic = N2kMsg.Get2ByteUDouble( 0.0001, Index );

    return true;
}

void SetN2kPGN65359( tN2kMsg &N2kMsg, uint8_t sid, double headingTrue, double headingMagnetic ) {
    N2kMsg.SetPGN( 65359L );
    //N2kMsg.Priority = 2;
    N2kMsg.Add2ByteUInt( RAYMARINE_MANUFACTURER_INDUSTRY );
    N2kMsg.AddByte( sid );
    N2kMsg.Add2ByteUDouble( headingTrue, 0.0001 );
    N2kMsg.Add2ByteUDouble( headingMagnetic, 0.0001 );
    N2kMsg.AddByte( 0 );
}

bool ParseN2kPGN65379( const tN2kMsg &N2kMsg, uint16_t &company, uint16_t &pilotMode, uint8_t &subMode,
                       uint8_t &pilotModeData ) {
    if ( N2kMsg.PGN != 65379L ) { return false; }

    int Index = 0;
    company = N2kMsg.Get2ByteUInt( Index );
    // 0: standby, 64: auto
    pilotMode = N2kMsg.Get2ByteUInt( Index );
    subMode = N2kMsg.GetByte( Index );
    pilotModeData = N2kMsg.GetByte( Index );
    // reserved

    return true;
}

bool ParseN2kPGN126720( const tN2kMsg &N2kMsg, int &Index, uint16_t &ManufacturerCode, uint8_t &Reserved,
                        uint8_t &IndustryCode, uint16_t &ProprietaryID ) {
    if ( N2kMsg.PGN != 126720L ) { return false; }

    Index = 0;
    uint16_t v = N2kMsg.Get2ByteUInt( Index );
    ManufacturerCode = v & ( ( 1 << 11 ) - 1 );
    Reserved = ( v >> 11 ) & 0x03;
    IndustryCode = ( v >> 13 ) & 0x07;
    ProprietaryID = N2kMsg.Get2ByteUInt( Index );

    return true;
}

void SetN2kPGN126720( tN2kMsg &N2kMsg, uint8_t pilotMode, uint8_t subMode, uint8_t pilotModeData ) {
    N2kMsg.SetPGN( 126720L );
    N2kMsg.Add2ByteUInt( RAYMARINE_MANUFACTURER_INDUSTRY );
    N2kMsg.Add2ByteUInt( SEATALK_PROPRIETARY_ID );
    N2kMsg.AddByte( 132 );
    N2kMsg.Add3ByteInt( 0 );
    /*
      if (
        (mode == 0 || mode == 'Standby' || mode == 68 || mode == 72) &&
        subMode == 0
      ) {
        return 'standby'
      } else if (
        mode == 'Wind' &&
        (subMode == 0 || subMode == 4 || subMode == 8 || subMode == 12)
      ) {
        // submodes: 0=on course,  4=off course pt/stb, 8=wind shift, submode 12 tbd
        return 'wind'
      } else if (mode == 'Track' && subMode == 0) {
        return 'route'
      } else if (mode == 'Auto' && (subMode == 0 || subMode == 4)) {
        //subMode 4 means offcourse
        return 'auto'
      }
     */
    // Pilot Mode
    N2kMsg.AddByte( pilotMode );
    // Sub Mode
    N2kMsg.AddByte( subMode );
    // Pilot Mode Data
    N2kMsg.AddByte( pilotModeData );
}

const char *SeaTalkKeystroke[10] = {
        "Auto",
        "Standby",
        "Wind",
        "-1",
        "-10",
        "+1",
        "+10",
        "-1 and -10",
        "+1 and +10",
        "Track",
};

const char *SeaTalkDeviceId[6] = {
        nullptr,
        nullptr,
        nullptr,
        "S100",
        nullptr,
        "Course Computer",
};

const char *SeaTalkNetworkGroup[11] = {
        "None",
        "Helm 1",
        "Helm 2",
        "Cockpit",
        "Flybridge",
        "Mast",
        "Group 1",
        "Group 2",
        "Group 3",
        "Group 4",
        "Group 5",
};

const char *SeaTalkDisplayColor[4] = {
        "Day 1",
        "Day 2",
        "Red/Black",
        "Inverse",
};

const LookupEntry SeaTalkPilotMode16[6] = {
        { 0,   "Standby" },
        { 1,   "Starting" },
        { 64,  "Auto, compass commanded" },
        { 256, "Vane, Wind Mode" },
        { 384, "Track Mode" },
        { 385, "No Drift, COG referenced (In track, course changes)" },
};

const char *SeaTalkAlarmStatus[3] = {
        "Alarm condition not met",
        "Alarm condition met and not silenced",
        "Alarm condition met and silenced"
};

const char *SeaTalkAlarmId[106] = {
        "No Alarm",
        "Shallow Depth",
        "Deep Depth",
        "Shallow Anchor",
        "Deep Anchor",
        "Off Course",
        "AWA High",
        "AWA Low",
        "AWS High",
        "AWS Low",
        "TWA High",
        "TWA Low",
        "TWS High",
        "TWS Low",
        "WP Arrival",
        "Boat Speed High",
        "Boat Speed Low",
        "Sea Temperature High",
        "Sea Temperature Low",
        "Pilot Watch",
        "Pilot Off Course",
        "Pilot Wind Shift",
        "Pilot Low Battery",
        "Pilot Last Minute Of Watch",
        "Pilot No NMEA Data",
        "Pilot Large XTE",
        "Pilot NMEA DataError",
        "Pilot CU Disconnected",
        "Pilot Auto Release",
        "Pilot Way Point Advance",
        "Pilot Drive Stopped",
        "Pilot Type Unspecified",
        "Pilot Calibration Required",
        "Pilot Last Heading",
        "Pilot No Pilot",
        "Pilot Route Complete",
        "Pilot Variable Text",
        "GPS Failure",
        "MOB",
        "Seatalk1 Anchor",
        "Pilot Swapped Motor Power",
        "Pilot Standby Too Fast To Fish",
        "Pilot No GPS Fix",
        "Pilot No GPS COG",
        "Pilot Start Up",
        "Pilot Too Slow",
        "Pilot No Compass",
        "Pilot Rate Gyro Fault",
        "Pilot Current Limit",
        "Pilot Way Point Advance Port",
        "Pilot Way Point Advance Stbd",
        "Pilot No Wind Data",
        "Pilot No Speed Data",
        "Pilot Seatalk Fail1",
        "Pilot Seatalk Fail2",
        "Pilot Warning Too Fast To Fish",
        "Pilot Auto Dockside Fail",
        "Pilot Turn Too Fast",
        "Pilot No Nav Data",
        "Pilot Lost Waypoint Data",
        "Pilot EEPROM Corrupt",
        "Pilot Rudder Feedback Fail",
        "Pilot Autolearn Fail1",
        "Pilot Autolearn Fail2",
        "Pilot Autolearn Fail3",
        "Pilot Autolearn Fail4",
        "Pilot Autolearn Fail5",
        "Pilot Autolearn Fail6",
        "Pilot Warning Cal Required",
        "Pilot Warning OffCourse",
        "Pilot Warning XTE",
        "Pilot Warning Wind Shift",
        "Pilot Warning Drive Short",
        "Pilot Warning Clutch Short",
        "Pilot Warning Solenoid Short",
        "Pilot Joystick Fault",
        "Pilot No Joystick Data",
        "Pilot Invalid Command",
        "AIS TX Malfunction",
        "AIS Antenna VSWR fault",
        "AIS Rx channel 1 malfunction",
        "AIS Rx channel 2 malfunction",
        "AIS No sensor position in use",
        "AIS No valid SOG information",
        "AIS No valid COG information",
        "AIS 12V alarm",
        "AIS 6V alarm",
        "AIS Noise threshold exceeded channel A",
        "AIS Noise threshold exceeded channel B",
        "AIS Transmitter PA fault",
        "AIS 3V3 alarm",
        "AIS Rx channel 70 malfunction",
        "AIS Heading lost/invalid",
        "AIS internal GPS lost",
        "AIS No sensor position",
        "AIS Lock failure",
        "AIS Internal GGA timeout",
        "AIS Protocol stack restart",
        "Pilot No IPS communications",
        "Pilot Power-On or Sleep-Switch Reset While Engaged",
        "Pilot Unexpected Reset While Engaged",
        "AIS Dangerous Target",
        "AIS Lost Target",
        "AIS Safety Related Message (used to silence)",
        "AIS Connection Lost",
        "No Fix",
};

const char *SeaTalkAlarmGroup[5] = {
        "Instrument",
        "Autopilot",
        "Radar",
        "Chart Plotter",
        "AIS"
};
