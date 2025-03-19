// src/symbol/tablestack.h

#pragma once

// #include "common.h"
// #include "table.h"
//
// typedef struct {
//     size_t ptr;                 // Points 1 above upper table
//     size_t capacity;
//     SymbolTable** tables;
// } SymbolTableStack;
//
// void STSpop(SymbolTableStack* sts);
// SymbolTableStack* STSnew();
// void STSfree(SymbolTableStack** sts_ptr);
// void STSpush(SymbolTableStack* sts, char* scope_name, ValueType ret_type);
// Symbol* STSlookup(const SymbolTableStack* sts, char* symbol_name);
// void STSadd(const SymbolTableStack* sts, char* symbol_name, Symbol* sym);
// char* STScurrentScopeName(const SymbolTableStack* sts);
// Symbol* STSlookupInTop(const SymbolTableStack* sts, char* symbol_name);
