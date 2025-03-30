// src/symbol/table.c

#include "table.h"

/**
 * Creates a new symboltable struct and initialises boilerplate
 * @param parent_table parent table of new symbol table (parent scope)
 * @param parent_symbol parent symbol of new symbol table (parent function)
 * @return pointer to new symbol table
 */
SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->localvar_offset_counter = 0;
    st->for_loop_counter = 0;
    st->nesting_level = parent_table == NULL ? 0 : parent_table->nesting_level + 1;
    st->parent_scope = parent_table;
    st->parent_fun = parent_symbol;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

/**
 * Frees symboltable and sets pointer to NULL
 * @param st_ptr double pointer to symboltable to free
 */
void STfree(SymbolTable** st_ptr) {
    SymbolTable* st = *st_ptr;

    // Loop through all symbols and delete them
    for (htable_iter_st *iter = HTiterate(st->table); iter;
            iter = HTiterateNext(iter)) {

        char* key = HTiterKey(iter);
        Symbol* value = HTiterValue(iter);

        MEMfree(key);
        key = NULL;
        SBfree(&value);
    }

    HTdelete(st->table);
    MEMfree(st);
    *st_ptr = NULL;
}

/**
 * Adds a new symbol to symboltable if it doesn't exist yet
 * @param st pointer to symbol table
 * @param name name of symbol to insert, functions as hashtable key
 * @param sym pointer to symbol to insert
 */
void STinsert(SymbolTable* st, char* name, Symbol* sym) {
    if (HTlookup(st->table, name) != NULL) {
        USER_ERROR("Symbol %s already exists, but is redefined", name);
        SBfree(&sym);
        return;
    }

#ifdef DEBUGGING
    ASSERT_MSG((sym->parent_scope == NULL), "Trying to assign scope to symbol, but it was already assigned");
#endif // DEBUGGING
    sym->parent_scope = st;
    HTinsert(st->table, STRcpy(name), sym);
}

/**
 * Finds a symbol in a symboltable. Does not search in parent scopes.
 * @param st pointer to symboltable
 * @param name name to look up in symboltable
 * @return symbol if found, otherwise NULL
 */
Symbol* STlookup(const SymbolTable* st, char* name) {
    return HTlookup(st->table, name);
}
