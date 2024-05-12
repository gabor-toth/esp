#ifndef HAJO_DISPLAY_COMMON_H
#define HAJO_DISPLAY_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FUEL,
    VOLTAGE,
    WATER,
    TEMP
} display_type_t;

extern const char *display_type_names[];

#ifdef __cplusplus
}
#endif

#endif //HAJO_DISPLAY_COMMON_H
