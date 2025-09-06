#ifndef ONTOZO_CONFIG_VERSION_H
#define ONTOZO_CONFIG_VERSION_H

#include <stdint.h>

typedef struct {
    int64_t version;
    char string[33];
    const char *nvs_key;
} ConfigVersion;

extern void config_version_set_and_write( uint32_t nvs_handle, ConfigVersion *version );

extern void config_version_read( uint32_t nvs_handle, const char *nvs_key, ConfigVersion *version );

#endif //ONTOZO_CONFIG_VERSION_H
