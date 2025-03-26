// src/types/types.h

#pragma once

/* Types declarations that are safe to import by other header files
 * without causing circular imports */

typedef enum {
    ST_VALUEVAR,
    ST_ARRAYVAR,
    ST_FUNCTION,
} SymbolType;

typedef enum ValueType {
    VT_NUM,
    VT_FLOAT,
    VT_BOOL,
    VT_VOID,
    VT_NUMARRAY,
    VT_FLOATARRAY,
    VT_BOOLARRAY,
    VT_NULL                             // Used to set as "no value top" within the code
} ValueType;
