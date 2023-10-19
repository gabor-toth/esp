#ifndef HAJO_DISPLAY_MAIN_H
#define HAJO_DISPLAY_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "display_common.h"

extern void display_main();

extern bool display_start_task();

extern void display_end_task();

extern void display_set_value( display_type_t type, int instance, int value );


#ifdef __cplusplus
}
#endif

#endif //HAJO_DISPLAY_MAIN_H
