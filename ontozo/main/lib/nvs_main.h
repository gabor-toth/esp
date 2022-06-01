#ifndef ONTOZO_NVS_MAIN_H
#define ONTOZO_NVS_MAIN_H

#include <stdint.h>

extern void nvs_init( void );

extern uint32_t nvs_open_storage();

extern char *nvs_read_string( const char *key );

extern void nvs_write_string( const char *key, const char *value );

#endif //ONTOZO_NVS_MAIN_H
