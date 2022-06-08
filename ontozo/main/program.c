#include "program.h"
#include <string.h>

Program *program_constructor() {
    Program *top = malloc( sizeof( Program ));
    memset( top, 0, sizeof( Program ));
    return top;
}

void program_destructor( Program *program ) {
    free( program->name );
    free( program->start_times );
    free( program->zones );
    free( program );
}
