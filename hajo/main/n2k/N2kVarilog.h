#ifndef HAJO_N2KVARILOG_H
#define HAJO_N2KVARILOG_H

#include "NMEA2000.h"
#include <cstdint>

// Just chosen free from code list on https://github.com/ieb/EngineMonitor/blob/master/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
#define N2K_MANUFACTURER_CODE_VARILOG   1725

typedef enum {
    EngineLightPressed,
    EngineLightReleased,
    EngineMainPressed,
    EngineMainReleased,
    EngineStartReleased,
    EngineStopPressed,
    EngineStopReleased,
} N2kVarilogEngineKeyPress;

#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS 0x0FF00 // 65208
#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS_INTERVAL_MS -1

#endif //HAJO_N2KVARILOG_H
