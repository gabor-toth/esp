#ifndef ONTOZO_PROGRAM_LOGIC_H
#define ONTOZO_PROGRAM_LOGIC_H

typedef struct {
    bool is_program_running;
    int program_index;
    int zone_index;
    int zone_left_seconds;
} RunningProgramState;

extern void program_logic_init();

extern void program_logic_start( int index );

extern void program_logic_move_to_next_zone();

extern void program_logic_move_to_next_program();

extern void program_logic_stop();

extern void program_logic_pump_state_change( bool is_on );

extern void program_logic_get_state( RunningProgramState *state );

extern int program_logic_get_queued_program( int queue_index );

#endif //ONTOZO_PROGRAM_LOGIC_H
