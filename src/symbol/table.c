// src/symbol/table.c

#include "table.h"

SymbolTable* STnew(char* name, const ValueType type) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->name = name;
    st->type = type;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

void STfree(SymbolTable** st) {
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
