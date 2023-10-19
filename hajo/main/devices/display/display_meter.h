#ifndef HAJO_DISPLAY_METER_H
#define HAJO_DISPLAY_METER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "display_common.h"

extern void display_meter_main_tabbed();
extern void display_set_value_tabbed( display_type_t type, int instance, int value );

extern void display_meter_main_single();
extern void display_set_value_single( display_type_t type, int instance, int value );

#ifdef __cplusplus
}
#endif

#endif //HAJO_DISPLAY_METER_H
