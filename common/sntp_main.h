#ifndef ONTOZO_SNTP_MAIN_H
#define ONTOZO_SNTP_MAIN_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void sntp_main();

extern bool sntp_is_time_set();

extern void local_time_to_buf( char *__restrict _s, size_t _maxsize );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_SNTP_MAIN_H
