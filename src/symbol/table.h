// src/symbol/table.h

#pragma once

#include "common.h"
#include "symbol.h"

/*
Struct SymbolTable keeps track of vars that are defined
*/
typedef struct SymbolTable {
    size_t localvar_offset_counter;     // Tracks offset for next variable
    size_t nesting_level;               // Nesting level; global is zero
    size_t for_loop_counter;            // Counter for for_loops
    struct SymbolTable* parent_scope;   // Pointer to parent scope
    Symbol* parent_fun;                 // Function this scope belongs to
                                        // Always points to a function, even if scope belongs to for-loop var
    htable_st* table;                   // Hashtable mapping symbol name to its properties
} SymbolTable;

SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol);
void STfree(SymbolTable** st_ptr);
void STinsert(SymbolTable* st, char* name, Symbol* sym);
Symbol* STlookup(const SymbolTable* st, char* name);
