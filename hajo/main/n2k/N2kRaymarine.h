#ifndef HAJO_N2KRAYMARINE_H
#define HAJO_N2KRAYMARINE_H

#include "N2kTypes.h"
#include "N2kMsg.h"

typedef struct {
    uint16_t value;
    const char *name;
} LookupEntry;

extern const char *SeaTalkAlarmGroup[5];
extern const char *SeaTalkAlarmId[106];
extern const char *SeaTalkAlarmStatus[3];
extern const char *SeaTalkDeviceId[6];
extern const char *SeaTalkDisplayColor[4];
extern const char *SeaTalkKeystroke[10];
extern const char *SeaTalkNetworkGroup[11];
extern const LookupEntry SeaTalkPilotMode16[6];

#define RAYMARINE_MANUFACTURER_INDUSTRY ((uint16_t)((4<<13)|(0<11)|(1851)))
#define SEATALK_PROPRIETARY_ID 33264

extern bool ParseN2kPGN61184( const tN2kMsg &N2kMsg, uint8_t &proprietaryID, uint8_t &variant, uint8_t &wirelessSetting,
                              uint8_t &wiredSetting, uint8_t &beepControl );

extern bool ParseN2kPGN65288( const tN2kMsg &N2kMsg, uint16_t &company, uint8_t &sid, uint8_t &alarmStatus,
                              uint8_t &alarmId, uint8_t &alarmGroup, uint8_t &alarmPriority );

extern bool ParseN2kPGN65359( const tN2kMsg &N2kMsg, uint8_t &sid, double &headingTrue,
                              double &headingMagnetic );

extern void SetN2kPGN65359( tN2kMsg &N2kMsg, uint8_t sid, double headingTrue,
                            double headingMagnetic );

extern bool ParseN2kPGN65360( const tN2kMsg &N2kMsg, uint16_t &company, uint8_t &sid, double &targetHeadingTrue,
                              double &targetHeadingMagnetic );

extern bool ParseN2kPGN65361( const tN2kMsg &N2kMsg, uint8_t &alarmId, uint8_t &alarmGroup );

extern bool ParseN2kPGN65379( const tN2kMsg &N2kMsg, uint16_t &company, uint16_t &pilotMode, uint8_t &subMode,
                              uint8_t &pilotModeData );

extern bool ParseN2kPGN126720( const tN2kMsg &N2kMsg, int &Index, uint16_t &ManufacturerCode, uint8_t &Reserved,
                               uint8_t &IndustryCode, uint16_t &ProprietaryID );

#define SEATALK_PILOT_MODE_Standby  64
#define SEATALK_PILOT_MODE_Auto     66
#define SEATALK_PILOT_MODE_Wind     70
#define SEATALK_PILOT_MODE_Track    74

extern void SetN2kPGN126720( tN2kMsg &N2kMsg, uint8_t pilotMode, uint8_t subMode, uint8_t pilotModeData );

// see https://github.com/AK-Homberger/NMEA2000-SeatalkNG-AlarmBuzzer/blob/master/NMEA2000-Alarm-Buzzer/NMEA2000-Alarm-Buzzer.ino
// see https://github.com/canboat/canboat/blob/master/analyzer/pgn.h

#define LOOKUP_SEATALK_ALARM_STATUS          \
  (",0=Alarm condition not met"              \
   ",1=Alarm condition met and not silenced" \
   ",2=Alarm condition met and silenced")

#define LOOKUP_SEATALK_ALARM_ID                                   \
  (",0=No Alarm"                                                  \
   ",1=Shallow Depth"                                             \
   ",2=Deep Depth"                                                \
   ",3=Shallow Anchor"                                            \
   ",4=Deep Anchor"                                               \
   ",5=Off Course"                                                \
   ",6=AWA High"                                                  \
   ",7=AWA Low"                                                   \
   ",8=AWS High"                                                  \
   ",9=AWS Low"                                                   \
   ",10=TWA High"                                                 \
   ",11=TWA Low"                                                  \
   ",12=TWS High"                                                 \
   ",13=TWS Low"                                                  \
   ",14=WP Arrival"                                               \
   ",15=Boat Speed High"                                          \
   ",16=Boat Speed Low"                                           \
   ",17=Sea Temp High"                                            \
   ",18=Sea Temp Low"                                             \
   ",19=Pilot Watch"                                              \
   ",20=Pilot Off Course"                                         \
   ",21=Pilot Wind Shift"                                         \
   ",22=Pilot Low Battery"                                        \
   ",23=Pilot Last Minute Of Watch"                               \
   ",24=Pilot No NMEA Data"                                       \
   ",25=Pilot Large XTE"                                          \
   ",26=Pilot NMEA DataError"                                     \
   ",27=Pilot CU Disconnected"                                    \
   ",28=Pilot Auto Release"                                       \
   ",29=Pilot Way Point Advance"                                  \
   ",30=Pilot Drive Stopped"                                      \
   ",31=Pilot Type Unspecified"                                   \
   ",32=Pilot Calibration Required"                               \
   ",33=Pilot Last Heading"                                       \
   ",34=Pilot No Pilot"                                           \
   ",35=Pilot Route Complete"                                     \
   ",36=Pilot Variable Text"                                      \
   ",37=GPS Failure"                                              \
   ",38=MOB"                                                      \
   ",39=Seatalk1 Anchor"                                          \
   ",40=Pilot Swapped Motor Power"                                \
   ",41=Pilot Standby Too Fast To Fish"                           \
   ",42=Pilot No GPS Fix"                                         \
   ",43=Pilot No GPS COG"                                         \
   ",44=Pilot Start Up"                                           \
   ",45=Pilot Too Slow"                                           \
   ",46=Pilot No Compass"                                         \
   ",47=Pilot Rate Gyro Fault"                                    \
   ",48=Pilot Current Limit"                                      \
   ",49=Pilot Way Point Advance Port"                             \
   ",50=Pilot Way Point Advance Stbd"                             \
   ",51=Pilot No Wind Data"                                       \
   ",52=Pilot No Speed Data"                                      \
   ",53=Pilot Seatalk Fail1"                                      \
   ",54=Pilot Seatalk Fail2"                                      \
   ",55=Pilot Warning Too Fast To Fish"                           \
   ",56=Pilot Auto Dockside Fail"                                 \
   ",57=Pilot Turn Too Fast"                                      \
   ",58=Pilot No Nav Data"                                        \
   ",59=Pilot Lost Waypoint Data"                                 \
   ",60=Pilot EEPROM Corrupt"                                     \
   ",61=Pilot Rudder Feedback Fail"                               \
   ",62=Pilot Autolearn Fail1"                                    \
   ",63=Pilot Autolearn Fail2"                                    \
   ",64=Pilot Autolearn Fail3"                                    \
   ",65=Pilot Autolearn Fail4"                                    \
   ",66=Pilot Autolearn Fail5"                                    \
   ",67=Pilot Autolearn Fail6"                                    \
   ",68=Pilot Warning Cal Required"                               \
   ",69=Pilot Warning OffCourse"                                  \
   ",70=Pilot Warning XTE"                                        \
   ",71=Pilot Warning Wind Shift"                                 \
   ",72=Pilot Warning Drive Short"                                \
   ",73=Pilot Warning Clutch Short"                               \
   ",74=Pilot Warning Solenoid Short"                             \
   ",75=Pilot Joystick Fault"                                     \
   ",76=Pilot No Joystick Data"                                   \
   ",77=not assigned"                                             \
   ",78=not assigned"                                             \
   ",79=not assigned"                                             \
   ",80=Pilot Invalid Command"                                    \
   ",81=AIS TX Malfunction"                                       \
   ",82=AIS Antenna VSWR fault"                                   \
   ",83=AIS Rx channel 1 malfunction"                             \
   ",84=AIS Rx channel 2 malfunction"                             \
   ",85=AIS No sensor position in use"                            \
   ",86=AIS No valid SOG information"                             \
   ",87=AIS No valid COG information"                             \
   ",88=AIS 12V alarm"                                            \
   ",89=AIS 6V alarm"                                             \
   ",90=AIS Noise threshold exceeded channel A"                   \
   ",91=AIS Noise threshold exceeded channel B"                   \
   ",92=AIS Transmitter PA fault"                                 \
   ",93=AIS 3V3 alarm"                                            \
   ",94=AIS Rx channel 70 malfunction"                            \
   ",95=AIS Heading lost/invalid"                                 \
   ",96=AIS internal GPS lost"                                    \
   ",97=AIS No sensor position"                                   \
   ",98=AIS Lock failure"                                         \
   ",99=AIS Internal GGA timeout"                                 \
   ",100=AIS Protocol stack restart"                              \
   ",101=Pilot No IPS communications"                             \
   ",102=Pilot Power-On or Sleep-Switch Reset While Engaged"      \
   ",103=Pilot Unexpected Reset While Engaged"                    \
   ",104=AIS Dangerous Target"                                    \
   ",105=AIS Lost Target"                                         \
   ",106=AIS Safety Related Message (used to silence)"            \
   ",107=AIS Connection Lost"                                     \
   ",108=No Fix")

#define LOOKUP_SEATALK_ALARM_GROUP \
  (",0=Instrument"                 \
   ",1=Autopilot"                  \
   ",2=Radar"                      \
   ",3=Chart Plotter"              \
   ",4=AIS")

#endif //HAJO_N2KRAYMARINE_H
