// src/variables/vartablestack.h

#pragma once

#include "common.h"
#include "palm/hash_table.h"
#include "variable.h"

typedef struct {
    int current;                                // Points one index above current height
    htable_st* stack[VARTABLE_STACK_SIZE];
} VarTableStack;

VarTableStack* VTSnew();
void VTSdestroy(VarTableStack* vts);
void VTSpush(VarTableStack* vts);
void VTSpop(VarTableStack* vts);
ScopeValue* VTSfind(const VarTableStack* vts, char* key);
void VTSadd(VarTableStack* vts, char* key, ScopeValue* value);
