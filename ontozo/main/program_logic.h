#ifndef ONTOZO_PROGRAM_LOGIC_H
#define ONTOZO_PROGRAM_LOGIC_H

typedef int64_t program_id_t;

typedef struct {
    bool is_program_running;
    program_id_t program_id;
    int program_index;
    int zone_index;
    int zone_left_seconds;
} RunningProgramState;

typedef struct {
    program_id_t program_id;
    int program_index;
    int zones_count;
    bool *zones_disabled;
} QueuedProgramState;

extern void program_logic_init();

extern void program_logic_start( int program_index );

extern void program_logic_move_to_next_zone( program_id_t program_id, int zone_index );

extern void program_logic_move_to_next_program( program_id_t program_id );

extern void program_logic_stop_all();

extern void program_logic_cancel_scheduled_program( program_id_t program_id );

extern void program_logic_toggle_scheduled_zone( program_id_t program_id, int zone_index );

extern void program_logic_pump_state_change( bool is_on );

extern int program_logic_get_queued_programs( RunningProgramState *running_state, QueuedProgramState **queue_state );

extern bool program_logic_is_program_in_use( int program_index );

#endif //ONTOZO_PROGRAM_LOGIC_H
