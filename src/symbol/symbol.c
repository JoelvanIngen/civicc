// src/symbol/symbol.c

#include "symbol.h"

#include "common.h"
#include "scopetree.h"

/**
 * Allocates memory for a new symbol and sets common attributes
 * @param name identifier name of the symbol
 * @param vt type of the value that the symbol holds or returns
 * @param imported boolean that sets whether the identifier was imported from an external file
 * @return pointer to new symbol struct
 */
static Symbol* SBnew(const char* name, const ValueType vt, const bool imported) {
    Symbol* s = MEMmalloc(sizeof(Symbol));
    s->vtype = vt;
    s->name = STRcpy(name);
    s->imported = imported;
    s->exported = false;
    s->parent_scope = NULL;
    return s;
}

/**
 * Creates a new symbol for a function identifier
 * @param name name of the function
 * @param vt return type of function
 * @param param_count amount of parameters the function expects
 * @param imported boolean that sets whether the function was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromFun(const char* name, const ValueType vt, const size_t param_count, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_FUNCTION;
    s->as.fun.label_name = NULL;
    s->as.fun.param_count = param_count;
    s->as.fun.param_ptr = 0;
    s->as.fun.param_types = MEMmalloc(sizeof(ValueType) * param_count);
    s->as.fun.param_dim_counts = MEMmalloc(sizeof(size_t) * param_count);
    return s;
}

/**
 * Creates a new symbol for an array variable
 * @param name name of the array variable
 * @param vt type of data the array holds
 * @param imported boolean that sets whether the array was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromArray(const char* name, const ValueType vt, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_ARRAYVAR;
    s->as.array.dim_count = 0;
    s->as.array.dims = NULL;
    return s;
}

/**
 * Creates a new symbol for a variable
 * @param name name of the variable
 * @param vt type of data the variable holds
 * @param imported boolean that sets whether the variable was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromVar(const char* name, const ValueType vt, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_VALUEVAR;
    return s;
}

/**
 * Creates a new symbol for a for-loop. This is a special symbol, containing useless data but
 * containing nested for-loop scope
 * @param adjusted_name name adjusted for for-loops so it cannot collide with user-generated names
 * @return pointer to new symbol struct
 */
Symbol* SBfromForLoop(const char* adjusted_name) {
    Symbol* s = SBnew(adjusted_name, VT_NULL, false);
    s->stype = ST_FORLOOP;
    return s;
}

/**
 * Frees a symbol struct and sets its pointer to NULL
 * @param s_ptr double pointer to symbol struct to free
 */
void SBfree(Symbol** s_ptr) {
    Symbol* s = *s_ptr;
    switch (s->stype) {
        case ST_FUNCTION:
            MEMfree(s->as.fun.label_name);
            MEMfree(s->as.fun.param_types);
            MEMfree(s->as.fun.param_dim_counts);
            if (!s->imported) STfree(&s->as.fun.scope);
            break;
        case ST_ARRAYVAR:
            if (s->as.array.dims != NULL) {
                // for (size_t i = 0; i < s->as.array.dim_count; i++) {
                //     if (s->as.array.dims[i] != NULL)
                //         SBfree(&s->as.array.dims[i]);
                // }
            }
            MEMfree(s->as.array.dims); break;
        case ST_FORLOOP:
            STfree(&s->as.forloop.scope); break;
        default: break;
    }

    MEMfree(s->name);
    MEMfree(s);
    *s_ptr = NULL;
}
