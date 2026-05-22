#ifndef HAJO_ENGINE_SENDER_INTERNAL_H
#define HAJO_ENGINE_SENDER_INTERNAL_H

#include "N2kMsg.h"

#define PIN_OUTPUT_MAIN      GPIO_NUM_3
#define PIN_OUTPUT_START     GPIO_NUM_5
#define PIN_OUTPUT_STOP      GPIO_NUM_7
#define PIN_OUTPUT_LIGHT     GPIO_NUM_9
#define PIN_OUTPUT_BUZZER_ENABLE    GPIO_NUM_11
#define PIN_OUTPUT_BUZZER_SOUND    GPIO_NUM_12

#define PIN_INPUT_OIL_SENSOR    GPIO_NUM_6
#define PIN_INPUT_TEMP_SENSOR   GPIO_NUM_8
#define PIN_INPUT_CHARGE_SENSOR GPIO_NUM_10
//#define PIN_INPUT_X_SENSOR    GPIO_NUM_13
#define PIN_INPUT_RPM_SENSOR    GPIO_NUM_14

#define RELAY_CLASS       0
#define SENSOR_CLASS      0
#define RPM_CLASS         1

#define PIN_INDEX_MAIN      0
#define PIN_INDEX_START     1
#define PIN_INDEX_STOP      2
#define PIN_INDEX_LIGHT     3
#define PIN_INDEX_BUZZER_ENABLE    4
#define PIN_INDEX_BUZZER_SOUND    5

#define PIN_INDEX_OIL_SENSOR      0
#define PIN_INDEX_TEMP_SENSOR     1
#define PIN_INDEX_CHARGE_SENSOR   2

#define MINIMAL_ENGINE_RPM 300

extern void setup_rpm(int ticks_per_revolution, double *engineSpeed);

extern void turn_main_on();

extern void setup_keys();

extern void process_engine_key_press(const tN2kMsg &N2kMsg);

typedef struct {
    uint8_t engineInstanceId;
    double engineSpeed;
    bool chargerFailure;
    bool oilPressureFailure;
    bool coolingWaterTemperatureFailure;
    bool mainOn;
    bool engineRunning;
    bool lightOn;
    bool starting;
    bool stopping;
    bool simulate;
    bool lastSidIsValid;
    uint8_t lastSid;
} EngineSenderData;

extern EngineSenderData engineSenderData;

#endif //HAJO_ENGINE_SENDER_INTERNAL_H
