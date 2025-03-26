// src/symbol/table.c

#include "table.h"

/* Create a new symbol table
 *
 * Output: Symbol table of type SymbolTable*
 *
 * Side-effects: Memory allocation
*/
SymbolTable* STnew(SymbolTable* parent_table, Symbol* parent_symbol) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->offset_counter = 0;
    st->for_loop_counter = 0;

    if (parent_table == NULL) {
        // Global scope
        st->nesting_level = 0;
    } else if (parent_symbol->stype == ST_FORLOOP) {
        // For-loop scope in the same function as parent
        st->nesting_level = parent_table->nesting_level;
    } else {
        // New function, higher nesting level
        st->nesting_level = parent_table->nesting_level + 1;
    }

    st->parent_scope = parent_table;
    st->parent_fun = parent_symbol;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

/* Free memory of the table inside SymbolTable**
 *
 * Output: None
 *
 * Side-effects: Freeing memory allocated for table.
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

/* Insert new value into symbol table if it does not exist
 *
 * st: Pointer to symbol table
 * name: Name of var to be added to table
 * sym: Struct containing all information of identifier
 *
 * Output: None
 * Side-effect: Added new identifier to table
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
    sym->offset = st->offset_counter;
    st->offset_counter++;
    sym->parent_scope = st;
    HTinsert(st->table, STRcpy(name), sym);
}

/* Lookup value in Symbol Table
 *
 * Output: Sym, containing all information about the identifier
 *
 * Side-effects: None.
*/
Symbol* STlookup(const SymbolTable* st, char* name) {
    return HTlookup(st->table, name);
}
