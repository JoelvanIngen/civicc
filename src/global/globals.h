#pragma once

#include <stdbool.h>
#include <stddef.h>

struct globals {
    int line;
    int col;
    int verbose;
    char *input_file;
    char *output_file;
};

extern struct SymbolTable* GB_GLOBAL_SCOPE;

extern struct globals global;
extern void GLBinitializeGlobals(void);
