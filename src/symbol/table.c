// src/symbol/table.c

#include "table.h"

/* Create a new symbol table
 *
 * Output: Symbol table of type SymbolTable*
 *
 * Side-effects: Memory allocation
*/
SymbolTable* STnew(char* name, const ValueType type) {
    SymbolTable* st = MEMmalloc(sizeof(SymbolTable));
    st->name = name;
    st->type = type;
    st->table = HTnew_String(VARTABLE_SIZE);
    return st;
}

/* Free memory of the table inside SymbolTable**
 *
 * Output: None
 *
 * Side-effects: Freeing memory allocated for table.
*/
void STfree(SymbolTable** st) {
    HTdelete((*st)->table);
    MEMfree(*st);
    *st = NULL;
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
void STinsert(const SymbolTable* st, char* name, Symbol* sym) {
    if (HTlookup(st->table, name) != NULL) {
        USER_ERROR("Symbol %s already exists, but is redefined", name);
        return;
    }
    HTinsert(st->table, name, sym);
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
