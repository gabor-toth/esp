#ifndef HAJO_TIMER_H
#define HAJO_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*timer_callback_t)(void* user_data);

void timer_start( const char* name, timer_callback_t callback, int interval_ms, void* user_data, bool tickOnCreate );

#ifdef __cplusplus
}
#endif

#endif //HAJO_TIMER_H
