// src/symbol/table.c

#include "table.h"

SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->parent_scope = parent_table;
    st->parent_fun = parent_symbol;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

// TODO: Free children symbols
void STfree(SymbolTable** st) {
    // Loop through all symbols and delete them

    HTdelete((*st)->table);
    MEMfree(*st);
    *st = NULL;
}

void STinsert(const SymbolTable* st, char* name, Symbol* sym) {
    if (HTlookup(st->table, name) != NULL) {
        USER_ERROR("Symbol %s already exists, but is redefined", name);
        return;
    }
    HTinsert(st->table, name, sym);
}

Symbol* STlookup(const SymbolTable* st, char* name) {
    return HTlookup(st->table, name);
}
