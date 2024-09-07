#ifndef HAJO_WIT_SENSOR_H
#define HAJO_WIT_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void wit_sensor_start();
extern bool wit_sensor_is_available();

extern void wit_sensor_get_pitch_and_roll( float *pitch, float *roll );

extern void wit_sensor_get_heading( float *heading );

#ifdef __cplusplus
}
#endif

#endif //HAJO_WIT_SENSOR_H
