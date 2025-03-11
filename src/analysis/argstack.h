// src/variables/paramstack.h

#pragma once

#include "../symbol/symbol.h"

typedef struct {
    ValueType type;
    const char* name;
    size_t arr_dim_count;
    size_t* arr_dims;
} Argument;

typedef struct {
    Argument* args;
    size_t ptr;
    size_t size;
} ArgList;

typedef struct {
    ArgList* fun_calls;
    size_t ptr;
    size_t size;
} ArgListStack;

ArgList* PLnew();

void ALadd(ArgList* pl, const char* name, ValueType type);

ArgListStack* ALSnew();
void ALSfree(ArgListStack** als);
void ALSpush(ArgListStack* als);
void ALSpop(ArgListStack* als);
void ALSadd(ArgListStack* als, const char* name, ValueType type);
Argument* ALSgetCurrentArgs(const ArgListStack* als);
size_t ALSgetCurrentLength(const ArgListStack* als);
