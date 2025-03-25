// src/symbol/table.h

#pragma once

#include "common.h"
#include "symbol.h"

/*
Struct SymbolTable keeps track of vars that are defined

name: Name of the callee
type: Type return value of callee
table: Contains key value pair for variables (key, symbol struct)
*/
typedef struct {
    char* name;       // Name of function that the scope belongs to, or NULL if global
    ValueType type;         // Return type that the function returns
    htable_st* table;       // Hashtable mapping symbol name to its properties
typedef struct SymbolTable {
    size_t offset_counter;              // Tracks offset for next variable
    size_t nesting_level;               // Nesting level; global is zero
    struct SymbolTable* parent_scope;   // Pointer to parent scope
    Symbol* parent_fun;                 // Function this scope belongs to
    htable_st* table;                   // Hashtable mapping symbol name to its properties
} SymbolTable;

SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol);
void STfree(SymbolTable** st_ptr);
void STinsert(SymbolTable* st, char* name, Symbol* sym);
Symbol* STlookup(const SymbolTable* st, char* name);
