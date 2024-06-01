#ifndef HAJO_DEBUG_HELPER_H
#define HAJO_DEBUG_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void debug_start_task_dump();

extern void debug_print_free_mem( const char *log );

#ifdef __cplusplus
}
#endif

#endif //HAJO_DEBUG_HELPER_H
