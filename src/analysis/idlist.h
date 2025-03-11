// src/analysis/idlist.h

#pragma once

#include "common.h"

typedef struct {
    size_t size;
    size_t capacity;
    const char** ids;
} IdList;

IdList* IDLnew();
void IDLfree(IdList** idl_ptr);
void IDLadd(IdList* idl, const char* id);
