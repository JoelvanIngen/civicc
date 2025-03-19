// src/symbol/table.h

#pragma once

#include "common.h"
#include "symbol.h"

typedef struct SymbolTable {
    size_t nesting_level;               // Nesting level; global is zero
    struct SymbolTable* parent_scope;   // Pointer to parent scope
    Symbol* parent_fun;                 // Function this scope belongs to
    htable_st* table;                   // Hashtable mapping symbol name to its properties
} SymbolTable;

SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol);
void STfree(SymbolTable** st_ptr);
void STinsert(const SymbolTable* st, char* name, Symbol* sym);
Symbol* STlookup(const SymbolTable* st, char* name);
