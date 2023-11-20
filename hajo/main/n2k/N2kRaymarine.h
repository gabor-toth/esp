#ifndef HAJO_N2KRAYMARINE_H
#define HAJO_N2KRAYMARINE_H

/*
1	Manufacturer Code	1851: Raymarine	0 .. 2045, 11 bits lookup MANUFACTURER_CODE
2	Reserved			2 bits RESERVED
3	Industry Code	4: Marine Industry 0 .. 6, 3 bits lookup INDUSTRY_CODE
4	Proprietary ID	33264: 0x81f0 0 .. 65533, 16 bits unsigned NUMBER
5	command	132: 0x84 0 .. 253, 8 bits unsigned NUMBER
6	Unknown 1			,24 bits BINARY
7	Pilot Mode 0 .. 253, 8 bits lookup SEATALK_PILOT_MODE
8	Sub Mode 0 .. 253, 8 bits unsigned NUMBER
9	Pilot Mode Data			,8 bits BINARY
10	Unknown 2			,80 bits BINARY
 */

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
