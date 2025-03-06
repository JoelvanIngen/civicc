// src/variables/paramstack.h
#pragma once

#include "variable.h"

typedef struct {
    VType type;
    const char* name;
} Parameter;

typedef struct {
    Parameter* args;
    size_t ptr;
    size_t size;
} ParamList;

typedef struct {
    ParamList* fun_calls;
    size_t ptr;
    size_t size;
} ParamListStack;

ParamList* PLnew();

void PLadd(ParamList* pl, const char* name, VType type);

ParamListStack* PLSnew();
void PLSfree(ParamListStack** pls);
void PLSpush(ParamListStack* pls);
void PLSpop(ParamListStack* pls);
void PLSadd(ParamListStack* pls, const char* name, VType type);
Parameter* PLSgetCurrentArgs(const ParamListStack* pls);
size_t PLSgetCurrentLength(const ParamListStack* pls);
