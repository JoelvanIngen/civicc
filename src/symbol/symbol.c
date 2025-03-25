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
static Symbol* SBnew(char* name, const ValueType vt, const bool imported) {
    Symbol* s = MEMmalloc(sizeof(Symbol));
    s->vtype = vt;
    s->name = STRcpy(name);
    s->imported = imported;
    s->parent_scope = NULL;
    return s;
}

/**
 * Creates a new symbol for a function identifier
 * @param name name of the function
 * @param vt return type of function
 * @param imported boolean that sets whether the function was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromFun(char* name, const ValueType vt, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_FUNCTION;
    s->as.fun.param_count = 0;
    s->as.fun.param_types = MEMmalloc(INITIAL_LIST_SIZE * sizeof(ValueType));
    s->as.fun.param_dim_counts = MEMmalloc(INITIAL_LIST_SIZE * sizeof(size_t));
    return s;
}

/**
 * Creates a new symbol for an array variable
 * @param name name of the array variable
 * @param vt type of data the array holds
 * @param imported boolean that sets whether the array was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromArray(char* name, const ValueType vt, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_ARRAYVAR;
    s->as.array.dim_count = 0;
    s->as.array.capacity = INITIAL_LIST_SIZE;
    s->as.array.dims = MEMmalloc(s->as.array.capacity * sizeof(size_t));
    return s;
}

 * Creates a new symbol for a variable
 * @param name name of the variable
 * @param vt type of data the variable holds
 * @param imported boolean that sets whether the variable was imported from an external file
 * @return pointer to new symbol struct
 */
Symbol* SBfromVar(char* name, const ValueType vt, const bool imported) {
    Symbol* s = SBnew(name, vt, imported);
    s->stype = ST_VALUEVAR;
    return s;
}

/* Free symbol struct for function or array. Vars don't need
 * their struct members free. See SBfromVar.abort
 *
 * s_ptr: Is of type &(s*) used to remove dangling vars and free memory.
 *
 * Output: None
 * Side-effects: Memory is freed for *s and dangling variable is resolved
 * by setting *s = NULL.
*/
void SBfree(Symbol** s_ptr) {
    Symbol* s = *s_ptr;
    switch (s->stype) {
        case ST_FUNCTION:
            MEMfree(s->as.fun.param_types);
            MEMfree(s->as.fun.param_dim_counts);
            STfree(&s->as.fun.scope);
            break;
        case ST_ARRAYVAR:
            MEMfree(s->as.array.dims); break;
        default: break;
    }

    MEMfree(s->name);
    MEMfree(s);
    *s_ptr = NULL;
}

/* Add dimension to array by reallocating and storing
 * dimension count in symbol struct.
 *
 * Additional information: The array is an 1d array
 * that functions as a multi-dimensional array.
 *
 * s: pointer to symbol struct
 * dim: new dimension
 *
 * Output: None
 * Side-effects: memory reallocated for new dimension of array.
*/
void SBaddDim(Symbol* s, size_t dim) {
#ifdef DEBUGGING
    ASSERT_MSG((s->stype == ST_ARRAYVAR), "Tried to add dimension for non-array symbol");
#endif
    if (s->as.array.dim_count + 1 >= s->as.array.capacity) {
        s->as.array.capacity *= 2;
        s->as.array.dims = MEMrealloc(s->as.array.dims, s->as.array.capacity * sizeof(size_t));
    }

    *s->as.array.dims[s->as.array.dim_count] = dim;
    s->as.array.dim_count++;
}

/* Add parameter to array of a symbol struct for a function. Check
 * SBfromFun.
 *
 * s: pointer to symbol struct of type function
 * vt: type of value to be added to array
 * dim_count: the current dimension of the array
 *
 * Output: None
 * Side-effects: If count greater than current capacity
 * increase the array size, and add new vallue to the array (params)
 * otherwise add value to params and increase param count.
*/
void SBaddParam(Symbol* s, ValueType const vt, size_t dim_count) {
#ifdef DEBUGGING
    ASSERT_MSG((s->stype == ST_FUNCTION), "Tried to add parameter for non-function symbol");
#endif
    if (s->as.fun.param_count + 1 >= s->as.fun.capacity) {
        s->as.fun.capacity *= 2;
        s->as.fun.param_types = MEMrealloc(s->as.fun.param_types, s->as.fun.capacity * sizeof(size_t));
        s->as.fun.param_dim_counts = MEMrealloc(s->as.fun.param_dim_counts, s->as.fun.capacity * sizeof(size_t));
    }

    s->as.fun.param_types[s->as.fun.param_count] = vt;
    s->as.fun.param_dim_counts[s->as.fun.param_count] = dim_count;
    s->as.fun.param_count++;
}
