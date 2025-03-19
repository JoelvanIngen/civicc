// src/analysis/idlist.h

#include "dimslist.h"

DimsList* DMLnew() {
    DimsList* dml = MEMmalloc(sizeof(DimsList));
    dml->size = 0;
    dml->capacity = VARTABLE_STACK_SIZE;
    dml->dims = MEMmalloc(dml->capacity * sizeof(size_t*));
    return dml;
}

void DMLfree(DimsList** dml_ptr) {
    DimsList* dml = *dml_ptr;
    MEMfree(dml->dims);
    MEMfree(dml);
    *dml_ptr = NULL;
}

void DMLadd(DimsList* dml, const size_t dim) {
    if (dml->size + 1 >= dml->capacity) {
        dml->capacity *= 2;
        ARRAY_RESIZE(dml->dims, dml->capacity);
    }

    *dml->dims[dml->size] = dim;
    dml->size++;
}
