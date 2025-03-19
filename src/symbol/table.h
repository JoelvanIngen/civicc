// src/symbol/table.h

#pragma once

#include "common.h"
#include "symbol.h"

typedef struct SymbolTable {
    struct SymbolTable* parent_scope;   // Pointer to parent scope
    Symbol* parent_fun;                 // Function this scope belongs to
    htable_st* table;                   // Hashtable mapping symbol name to its properties
} SymbolTable;

SymbolTable* STnew(char* name, ValueType type);
void STfree(SymbolTable** st);
void STinsert(const SymbolTable* st, char* name, Symbol* sym);
Symbol* STlookup(const SymbolTable* st, char* name);
