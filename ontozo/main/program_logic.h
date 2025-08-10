#ifndef ONTOZO_PROGRAM_LOGIC_H
#define ONTOZO_PROGRAM_LOGIC_H

typedef struct {
    bool is_program_running;
    int program_index;
    int zone_index;
    int zone_left_seconds;
    int zones_count;
} RunningProgramState;

extern void program_logic_init();

extern void program_logic_start( int index );

extern void program_logic_move_to_next_zone();

extern void program_logic_move_to_next_program();

extern void program_logic_stop();

extern void program_logic_get_state( RunningProgramState *state );

#endif //ONTOZO_PROGRAM_LOGIC_H
