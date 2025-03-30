// src/symbol/scopetree.c

#include "scopetree.h"

/**
 * Looks up the name string in the table tree
 * @param scope SymbolTable to start lookup from
 * @param name name of the symbol that we want to find
 * @return symbol if found in table or any parent, else NULL
 */
Symbol* ScopeTreeFind(const SymbolTable* scope, char* name) {
#ifdef DEBUGGING
    ASSERT_MSG((scope != NULL), "Got NULL for variable scope");
#endif // DEBUGGING
    while (scope != NULL) {
        Symbol* s = STlookup(scope, name);
        if (s != NULL) return s;

        scope = scope->parent_scope;
    }

    return NULL;
}
