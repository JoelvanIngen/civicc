// src/analysis/idlist.h

#include "idlist.h"

IdList* IDLnew() {
    IdList* idl = MEMmalloc(sizeof(IdList));
    idl->ptr = 0;
    idl->capacity = VARTABLE_STACK_SIZE;
    idl->ids = MEMmalloc(idl->capacity * sizeof(char*));
    return idl;
}

void IDLfree(IdList** idl_ptr) {
    IdList* idl = *idl_ptr;

    for (size_t i = 0; i < idl->ptr; i++) {
        MEMfree(idl->ids[i]);
    }

    MEMfree(idl->ids);
    MEMfree(idl);
    *idl_ptr = NULL;
}

void IDLadd(IdList* idl, char* id) {
    if (idl->ptr + 1 >= idl->capacity) {
        idl->capacity *= 2;
        ARRAY_RESIZE(idl->ids, idl->capacity);
    }

    idl->ids[idl->ptr] = STRcpy(id);
    idl->ptr++;
}
