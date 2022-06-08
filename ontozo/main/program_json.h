#ifndef ONTOZO_PROGRAM_JSON_H
#define ONTOZO_PROGRAM_JSON_H

#include "program.h"

extern esp_err_t program_read_from_json( cJSON *root, Program **program_out );

extern esp_err_t program_write_to_json( Program *program, char **json_out );

#endif //ONTOZO_PROGRAM_JSON_H
