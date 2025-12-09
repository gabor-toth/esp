#ifndef HAJO_N2KVARILOG_H
#define HAJO_N2KVARILOG_H

#include "NMEA2000.h"
#include <cstdint>

// Just chosen free from code list on https://github.com/ieb/EngineMonitor/blob/master/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
#define N2K_MANUFACTURER_CODE_VARILOG   1725

/**********************************
 *
 * EngineKeyPress
 *
 **********************************/

#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS 0x0FF00            // 65208
#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS_INTERVAL_MS (-1)

typedef union {
    struct {
        unsigned main: 1;
        unsigned start: 1;
        unsigned stop: 1;
        unsigned light: 1;
    } Keys;

    uint8_t ByteValue;
} N2kVarilogEngineKeys;

typedef struct {
    uint8_t instanceId;
    uint8_t sid;
    N2kVarilogEngineKeys keysPressed;
    N2kVarilogEngineKeys keysChanged;
} N2kPGNVarilogEngineKeyPress;

extern bool ParseN2kPGNVarilogEngineKeyPress(const tN2kMsg &N2kMsg, uint8_t &instanceId, uint8_t &sid,
                                             N2kVarilogEngineKeys &keysPressed, N2kVarilogEngineKeys &keysChanged);

extern bool ParseN2kPGNVarilogEngineKeyPress(const tN2kMsg &N2kMsg, N2kPGNVarilogEngineKeyPress &data);

extern void SetN2kPGNVarilogEngineKeyPress(
    tN2kMsg &N2kMsg,
    uint8_t instanceId,
    uint8_t sid,
    N2kVarilogEngineKeys keysPressed,
    N2kVarilogEngineKeys keysChanged);

extern void SetN2kPGNVarilogEngineKeyPress(tN2kMsg &N2kMsg, const N2kPGNVarilogEngineKeyPress &data);

/**********************************
 *
 * EngineKeyPressAck
 *
 **********************************/

#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK 0x0FF01            // 65209
#define N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK_INTERVAL_MS (-1)

typedef struct {
    uint8_t instanceId;
    uint8_t sid;
} N2kPGNVarilogEngineKeyPressAck;

extern bool ParseN2kPGNVarilogEngineKeyPressAck(const tN2kMsg &N2kMsg, uint8_t &instanceId, uint8_t &sid);

extern bool ParseN2kPGNVarilogEngineKeyPressAck(const tN2kMsg &N2kMsg, N2kPGNVarilogEngineKeyPressAck &data);

extern void SetN2kPGNVarilogEngineKeyPressAck(
    tN2kMsg &N2kMsg,
    uint8_t instanceId,
    uint8_t sid);

extern void SetN2kPGNVarilogEngineKeyPressAck(tN2kMsg &N2kMsg, const N2kPGNVarilogEngineKeyPressAck &data);

/**********************************
 *
 * EngineState
 *
 **********************************/

#define N2K_PGN_VARILOG_ENGINE_STATE 0x0FF02            // 65210
#define N2K_PGN_VARILOG_ENGINE_STATE_INTERVAL_MS (-1)

typedef struct {
    uint8_t instanceId;
    bool engineOn;
} N2kPGNVarilogEngineState;

extern bool ParseN2kPGNVarilogEngineState(const tN2kMsg &N2kMsg, N2kPGNVarilogEngineState &data);

extern void SetN2kPGNVarilogEngineState(
    tN2kMsg &N2kMsg,
    uint8_t instanceId,
    bool engineOn);

extern void SetN2kPGNVarilogEngineState(tN2kMsg &N2kMsg, const N2kPGNVarilogEngineState &data);

/**********************************
 *
 * EngineSetState
 *
 **********************************/

#define N2K_PGN_VARILOG_ENGINE_SET_STATE 0x0FF03           // 65211
#define N2K_PGN_VARILOG_ENGINE_SET_STATE_INTERVAL_MS (-1)

extern bool ParseN2kPGNVarilogEngineSetState(const tN2kMsg &N2kMsg, N2kPGNVarilogEngineState &data);

extern void SetN2kPGNVarilogEngineSetState(
    tN2kMsg &N2kMsg,
    uint8_t instanceId,
    bool engineOn);

extern void SetN2kPGNVarilogEngineSetState(tN2kMsg &N2kMsg, const N2kPGNVarilogEngineState &data);

#endif //HAJO_N2KVARILOG_H
