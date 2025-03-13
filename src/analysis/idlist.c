// src/analysis/idlist.h

#include "idlist.h"

IdList* IDLnew() {
    IdList* idl = MEMmalloc(sizeof(IdList));
    idl->size = 0;
    idl->capacity = VARTABLE_STACK_SIZE;
    idl->ids = MEMmalloc(idl->capacity * sizeof(char*));
    return idl;
}

// Does NOT free the ID array, since it is meant to be re-assigned
// to different, existing struct
void IDLfree(IdList** idl_ptr) {
    IdList* idl = *idl_ptr;
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
