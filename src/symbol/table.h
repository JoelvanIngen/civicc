// src/symbol/table.h

#pragma once

#include "common.h"
#include "symbol.h"

typedef struct {
    char* name;       // Name of function that the scope belongs to, or NULL if global
    ValueType type;         // Return type that the function returns
    htable_st* table;       // Hashtable mapping symbol name to its properties
} SymbolTable;

SymbolTable* STnew(char* name, ValueType type);
void STfree(SymbolTable** st);
void STinsert(const SymbolTable* st, char* name, Symbol* sym);
Symbol* STlookup(const SymbolTable* st, char* name);
