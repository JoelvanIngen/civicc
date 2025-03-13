// src/symbol/symbol.c

#include "symbol.h"

#include "common.h"

static Symbol* SBnew() {
    return MEMmalloc(sizeof(Symbol));
}

Symbol* SBfromFun(const char* name, const ValueType vt) {
    Symbol* s = SBnew();
    s->stype = ST_FUNCTION;
    s->vtype = vt;
    s->name = name;
    s->as.fun.param_count = 0;
    s->as.fun.param_types = MEMmalloc(INITIAL_LIST_SIZE * sizeof(ValueType));
    return s;
}

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

Symbol* SBfromVar(const char* name, const ValueType vt) {
    Symbol* s = SBnew();
    s->stype = ST_VALUEVAR;
    s->vtype = vt;
    s->name = name;
    return s;
}

void SBfree(Symbol** s) {
    switch ((*s)->stype) {
        case ST_FUNCTION:
            MEMfree((*s)->as.fun.param_types); break;
        case ST_ARRAYVAR:
            MEMfree((*s)->as.array.dims); break;
        default: break;
    }

    MEMfree(*s);
    *s = NULL;
}

void SBaddDim(Symbol* s, size_t dim) {
#ifdef DEBUGGING
    // TODO: Fix macro to not need varglist
    ASSERT_MSG((s->stype == ST_ARRAYVAR), "Tried to add dimension for non-array symbol%s", "");
#endif
    if (s->as.array.dim_count + 1 >= s->as.array.capacity) {
        s->as.array.capacity *= 2;
        s->as.array.dims = MEMrealloc(s->as.array.dims, s->as.array.capacity * sizeof(size_t));
    }

    *s->as.array.dims[s->as.array.dim_count] = dim;
    s->as.array.dim_count++;
}

void SBaddParam(Symbol* s, ValueType const vt) {
#ifdef DEBUGGING
    // TODO: Fix macro to not need varglist
    ASSERT_MSG((s->stype == ST_FUNCTION), "Tried to add parameter for non-function symbol%s", "");
#endif
    if (s->as.fun.param_count + 1 >= s->as.fun.capacity) {
        s->as.fun.capacity *= 2;
        s->as.fun.param_types = MEMrealloc(s->as.fun.param_types, s->as.fun.capacity * sizeof(size_t));
    }

    s->as.fun.param_types[s->as.fun.param_count] = vt;
    s->as.fun.param_count++;
}
