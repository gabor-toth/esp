#ifndef LIB_REST_MAIN_H
#define LIB_REST_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rest_server.h"

extern void rest_init_before_wifi( void );

extern void rest_init_after_wifi( rest_register_handlers_t rest_register_handlers );

#ifdef __cplusplus
}
#endif

#endif //LIB_REST_MAIN_H
