// src/symbol/symbol.c

#include "symbol.h"

#include "common.h"


/* Create new symbol struct to keep track of symbol information
 *
 * Output: pointer to allocated memory for struct
*/
static Symbol* SBnew() {
    return MEMmalloc(sizeof(Symbol));
}

/* Create a symbol struct as function and initialize space for
 * function parameters
 *
 * name: name of function
 * vt: Value type enum value which defines the type
 *
 * Output: Pointer to symbol struct for a function
*/
Symbol* SBfromFun(const char* name, const ValueType vt) {
    Symbol* s = SBnew();
    s->stype = ST_FUNCTION;
    s->vtype = vt;
    s->name = name;
    s->as.fun.param_count = 0;
    s->as.fun.param_types = MEMmalloc(INITIAL_LIST_SIZE * sizeof(ValueType));
    s->as.fun.param_dim_counts = MEMmalloc(INITIAL_LIST_SIZE * sizeof(size_t));
    return s;
}

/* Create a symbol struct as array and initialize space for
 * array values
 *
 * name: name of array variable
 * vt: type of values within array
 *
 * Output: Pointer to symbol struct for an array
*/
Symbol* SBfromArray(const char* name, const ValueType vt) {
    Symbol* s = SBnew();
    s->stype = ST_ARRAYVAR;
    s->vtype = vt;
    s->name = name;
    s->as.array.dim_count = 0;
    s->as.array.capacity = INITIAL_LIST_SIZE;
    s->as.array.dims = MEMmalloc(s->as.array.capacity * sizeof(size_t));
    return s;
}

/* Create a symbol struct as variable and no space is initialized
 * but for the symbol symbol itself
 *
 * name: name of array variable
 * vt: type of values within array
 *
 * Output: Pointer to symbol struct for an array
*/
Symbol* SBfromVar(const char* name, const ValueType vt) {
    Symbol* s = SBnew();
    s->stype = ST_VALUEVAR;
    s->vtype = vt;
    s->name = name;
    return s;
}

/* Free symbol struct for function or array. Vars don't need
 * their struct members free. See SBfromVar.abort
 *
 * s: Is of type &(s*) used to remove dangling vars and free memory.
 *
 * Output: None
 * Side-effects: Memory is freed for *s and dangling variable is resolved
 * by setting *s = NULL.
*/
void SBfree(Symbol** s) {
    switch ((*s)->stype) {
        case ST_FUNCTION:
            MEMfree((*s)->as.fun.param_types);
            MEMfree((*s)->as.fun.param_dim_counts);
            break;
        case ST_ARRAYVAR:
            MEMfree((*s)->as.array.dims); break;
        default: break;
    }

    MEMfree(*s);
    *s = NULL;
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
