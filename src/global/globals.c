#include "globals.h"

#include "symbol/table.h"

struct globals global;

SymbolTable* GB_GLOBAL_SCOPE;

/*
 * Initialize global variables from globals.mac
 */

void GLBinitializeGlobals()
{
    global.col = 0;
    global.line = 0;
    global.input_file = NULL;
    global.output_file = NULL;
}
