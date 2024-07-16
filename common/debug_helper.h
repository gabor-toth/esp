#ifndef HAJO_DEBUG_HELPER_H
#define HAJO_DEBUG_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"
#include "stdint.h"

extern void debug_start_task_dump( uint16_t period_sec );

extern void debug_start_heap_dump( uint16_t period_sec );

extern size_t debug_print_free_mem( const char *log );

#ifdef __cplusplus
}
#endif

#endif //HAJO_DEBUG_HELPER_H
