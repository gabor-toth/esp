#ifndef HAJO_DISPLAY_METER_H
#define HAJO_DISPLAY_METER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "display_common.h"

extern void display_meter_main();

extern void display_meter_set_value( display_type_t type, int instance, int value );

#ifdef __cplusplus
}
#endif

#endif //HAJO_DISPLAY_METER_H
