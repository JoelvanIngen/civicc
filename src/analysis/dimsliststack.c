// src/symbol/tablestack.c

#include "dimsliststack.h"

typedef DimsListStack DLS;

static DimsList* top(const DLS* dls) {
    return dls->dimslists[dls->ptr - 1];
}

void DLSpop(DLS* dls) {
    dls->ptr--;
    DMLfree(&dls->dimslists[dls->ptr]);
    dls->dimslists[dls->ptr] = NULL;
}

DLS* DLSnew() {
    DLS* dls = MEMmalloc(sizeof(DLS));
    dls->ptr = 0;
    dls->capacity = VARTABLE_STACK_SIZE;
    dls->dimslists = MEMmalloc(VARTABLE_STACK_SIZE * sizeof(DimsList*));
    return dls;
}

void DLSfree(DLS** dls_ptr) {
    DLS* dls = *dls_ptr;

    while (dls->ptr > 0) {
        DimsList* entry = top(dls);
        DMLfree(&entry);
    }

    dls->ptr = 0;
    dls->capacity = 0;
    MEMfree(dls->dimslists);
    MEMfree(dls);
    *dls_ptr = NULL;
}

void DLSpush(DLS* dls) {
    if (dls->ptr >= dls->capacity) {
        dls->capacity *= 2;
        dls->dimslists = MEMrealloc(dls->dimslists, dls->capacity * sizeof(DimsList*));
    }

    dls->dimslists[dls->ptr] = DMLnew();
    dls->ptr++;
}

void DLSadd(DLS* dls, size_t dim) {
    DimsList* dml = top(dls);
#ifdef DEBUGGING
    ASSERT_MSG((dml != NULL), "No DML initialised on the DMS");
#endif
    DMLadd(dml, dim);
}

DimsList* DLSpeekTop(DLS* dls) {
    return top(dls);
}