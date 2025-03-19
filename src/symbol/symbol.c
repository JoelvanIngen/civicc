// src/symbol/symbol.c

#include "symbol.h"

#include "common.h"
#include "scopetree.h"

static Symbol* SBnew(char* name, const ValueType vt) {
    Symbol* s = MEMmalloc(sizeof(Symbol));
    s->vtype = vt;
    s->name = STRcpy(name);
    s->parent_scope = NULL;
    return s;
}

Symbol* SBfromFun(char* name, const ValueType vt) {
    Symbol* s = SBnew(name, vt);
    s->stype = ST_FUNCTION;
    s->as.fun.param_count = 0;
    s->as.fun.param_types = MEMmalloc(INITIAL_LIST_SIZE * sizeof(ValueType));
    s->as.fun.param_dim_counts = MEMmalloc(INITIAL_LIST_SIZE * sizeof(size_t));
    return s;
}

Symbol* SBfromArray(char* name, const ValueType vt) {
    Symbol* s = SBnew(name, vt);
    s->stype = ST_ARRAYVAR;
    s->as.array.dim_count = 0;
    s->as.array.capacity = INITIAL_LIST_SIZE;
    s->as.array.dims = MEMmalloc(s->as.array.capacity * sizeof(size_t));
    return s;
}

Symbol* SBfromVar(char* name, const ValueType vt) {
    Symbol* s = SBnew(name, vt);
    s->stype = ST_VALUEVAR;
    return s;
}

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
