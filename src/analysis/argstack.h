// src/variables/paramstack.h

#pragma once

#include "../symbol/symbol.h"

typedef struct {
    ValueType type;
    size_t arr_dim_count;
    size_t* arr_dims;
} Argument;

typedef struct {
    Argument* args;
    size_t ptr;
    size_t capacity;
} ArgList;

typedef struct {
    ArgList** fun_calls;
    size_t ptr;
    size_t capacity;
} ArgListStack;

void ALadd(ArgList* al, ValueType type);

ArgListStack* ALSnew();
void ALSfree(ArgListStack** als);
void ALSpush(ArgListStack* als);
void ALSpop(ArgListStack* als);
void ALSadd(ArgListStack* als, ValueType type);
Argument** ALSgetCurrentArgs(const ArgListStack* als);
size_t ALSgetCurrentLength(const ArgListStack* als);
