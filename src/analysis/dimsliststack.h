// src/symbol/tablestack.h

#pragma once

#include "common.h"
#include "dimslist.h"

typedef struct {
    size_t ptr;                 // Points 1 above upper list
    size_t capacity;
    DimsList** dimslists;
} DimsListStack;

void DLSpop(DimsListStack* dls);
DimsListStack* DLSnew();
void DLSfree(DimsListStack** dls_ptr);
void DLSpush(DimsListStack* dls);
void DLSadd(DimsListStack* dls, const size_t dim);
DimsList* DLSpeekTop(DimsListStack* dls);
