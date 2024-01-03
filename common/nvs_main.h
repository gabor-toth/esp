#ifndef ONTOZO_NVS_MAIN_H
#define ONTOZO_NVS_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <nvs.h>

extern void nvs_init( void );

extern nvs_handle_t nvs_open_storage();

extern void nvs_close_storage( nvs_handle_t nvs_handle );

extern char *nvs_read_string( nvs_handle_t nvs_handle, const char *key );

extern void nvs_open_and_write_string( const char *key, const char *value );

extern void nvs_write_string( nvs_handle_t nvs_handle, const char *key, const char *value );

extern void nvs_delete( nvs_handle_t nvs_handle, const char *key );

#ifdef __cplusplus
}
#endif

#endif //ONTOZO_NVS_MAIN_H
