#ifndef ONTOZO_PROGRAM_H
#define ONTOZO_PROGRAM_H

#include <sys/time.h>
#include <stdbool.h>

typedef struct {
    int zone_id;
    int duration_in_seconds;
} ProgramZone;

typedef enum {
    unused = 0, on, interval
} ProgramDayType;

typedef struct {
    ProgramDayType type;
    int on_days;
    int interval_days;
    int interval_start_day;
    bool interval_start_reset;
} ProgramDay;

typedef struct {
    char *name;
    int zones_count;
    ProgramZone *zones;
    int start_times_count;
    int *start_times;
    ProgramDay days;
    time_t last_run_time;
    time_t next_run_time;
} Program;

extern Program *program_constructor();

extern void program_destructor( Program *program );

#endif //ONTOZO_PROGRAM_H
