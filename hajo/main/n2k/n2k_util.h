#ifndef HAJO_N2K_UTIL_H
#define HAJO_N2K_UTIL_H

#include "NMEA2000.h"

extern uint32_t n2k_get_device_id();

template<typename T>
void n2k_incoming_value( T newValue, T &currentValue, bool &changed ) {
    if ( newValue != currentValue ) {
        currentValue = newValue;
        changed = true;
    }
}

#endif //HAJO_N2K_UTIL_H
