#ifndef ONTOZO_PROGRAM_H
#define ONTOZO_PROGRAM_H

#include <sys/time.h>
#include <stdbool.h>
#include <esp_err.h>
#include "cJSON.h"

#define PROGRAM_START_TIME( H, M ) ((H) * 100 + (M))
#define PROGRAM_ON_DAY( MASK, D ) ((MASK) &(1<<((D)+1)))

typedef struct {
    int zone_id;
    int duration_in_seconds;
} ProgramZone;

typedef enum {
    unused = 0, onDays, interval
} ProgramDayType;

typedef struct {
    ProgramDayType type;
    int on_days;
    int interval_days;
    int interval_start_day;
    bool interval_start_reset;
} ProgramDay;

typedef struct {
    int index;
    bool valid;
    bool enabled;
    char *name;
    int zones_count;
    ProgramZone *zones;
    int start_times_count;
    int *start_times;
    ProgramDay days;
    time_t last_run_time;
    time_t next_run_time;
} Program;

extern void program_init();

extern void program_add( Program *program );

extern void program_change( Program *program );

extern esp_err_t program_delete( int index );

extern int program_get_count();

extern Program *program_get( int index );

extern Program *program_constructor();

extern void program_destructor( Program *program );

#endif //ONTOZO_PROGRAM_H
