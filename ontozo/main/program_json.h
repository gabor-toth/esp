#ifndef ONTOZO_PROGRAM_JSON_H
#define ONTOZO_PROGRAM_JSON_H

#include "program.h"

extern esp_err_t program_read_from_string( const char *json_string, Program **program_out );

extern esp_err_t program_read_from_json( cJSON *root, Program **program_out );

extern esp_err_t program_write_to_string( Program *program, char **json_out );

extern esp_err_t programs_header_write_to_json( char **json_out );

#endif //ONTOZO_PROGRAM_JSON_H
