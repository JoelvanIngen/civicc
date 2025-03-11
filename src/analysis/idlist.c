// src/analysis/idlist.h

#include "idlist.h"

IdList* IDLnew() {
    IdList* idl = MEMmalloc(sizeof(IdList));
    idl->size = 0;
    idl->capacity = VARTABLE_STACK_SIZE;
    idl->ids = MEMmalloc(idl->capacity * sizeof(char*));
    return idl;
}

void IDLfree(IdList** idl_ptr) {
    IdList* idl = *idl_ptr;
    MEMfree(idl->ids);
    MEMfree(idl);
    *idl_ptr = NULL;
}

void IDLadd(IdList* idl, const char* id) {
    if (idl->size + 1 >= idl->capacity) {
        idl->capacity *= 2;
        ARRAY_RESIZE(idl->ids, idl->capacity);
    }

    idl->ids[idl->size] = id;
    idl->size++;
}
