//src/analysis/table.c

#include "variable.h"

#include <palm/hash_table.h>

#include "palm/memory.h"

ScopeValue* TEcreate() {
    return MEMmalloc(sizeof(ScopeValue));
};

void TEdestroy(ScopeValue** sv) {
    switch ((*sv)->data_t) {
        case NUM_ARRAY:
        case FLOAT_ARRAY:
        case BOOL_ARRAY:
            MEMfree((*sv)->as.array.data);
            MEMfree((*sv)->as.array.dims);
            break;
        default:
            break;
    }

    MEMfree(*sv);
    *sv = NULL;
}

bool is_array(const ScopeValue* te) {
    switch (te->data_t) {
        case NUM_ARRAY:
        case FLOAT_ARRAY:
        case BOOL_ARRAY:
            return true;
        default:
            return false;
    }
}
