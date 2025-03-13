// src/analysis/idlist.h

#pragma once

#include "common.h"

typedef struct {
    size_t size;
    size_t capacity;
    size_t** dims;
} DimsList;

DimsList* DMLnew();
void DMLfree(DimsList** dml_ptr);
void DMLadd(DimsList* dml, size_t dim);
