// src/symbol/table.c

#include "table.h"

SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->nesting_level = parent_table == NULL ? 0 : parent_table->nesting_level + 1;
    st->parent_scope = parent_table;
    st->parent_fun = parent_symbol;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

void STfree(SymbolTable** st_ptr) {
    SymbolTable* st = *st_ptr;

    // Loop through all symbols and delete them
    for (htable_iter_st *iter = HTiterate(st->table); iter;
            iter = HTiterateNext(iter)) {

        void *key = HTiterKey(iter);
        MEMfree(key);
        key = NULL;
        void *value = HTiterValue(iter);
        SBfree((Symbol**) &value);
    }

    HTdelete(st->table);
    MEMfree(st);
    *st_ptr = NULL;
}

void STinsert(const SymbolTable* st, char* name, Symbol* sym) {
    if (HTlookup(st->table, name) != NULL) {
        USER_ERROR("Symbol %s already exists, but is redefined", name);
        return;
    }

#ifdef DEBUGGING
    ASSERT_MSG((sym->scope == NULL), "Trying to assign scope to symbol, but it was already assigned");
#endif // DEBUGGING
    HTinsert(st->table, STRcpy(name), sym);
}

Symbol* STlookup(const SymbolTable* st, char* name) {
    return HTlookup(st->table, name);
}
