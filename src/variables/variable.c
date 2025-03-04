//src/variables/variable.c

#include "variable.h"

#include "palm/hash_table.h"
#include "palm/memory.h"

#define SV ScopeValue  // Makes definitions shorter

static size_t elem_size(const VType type) {
    switch (type) {
        case NUM:
        case NUM_ARRAY: return sizeof(int64_t);
        case FLOAT:
        case FLOAT_ARRAY: return sizeof(double);
        case BOOL:
        case BOOL_ARRAY: return sizeof(bool);
        case VOID: return 0;
        default: return -1; // TODO: ERROR
    }
}

static SV* SVcreate() {
    return MEMmalloc(sizeof(SV));
}

SV* SVfromFun(const VType vtype) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == NUM || vtype == FLOAT || vtype == BOOL || vtype == VOID),
        "Function return type (%i) is not num, float, bool or void", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = FN;
    return sv;
}

SV* SVfromVar(const VType vtype, const void* value) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == NUM || vtype == FLOAT || vtype == BOOL),
        "Var type (%i) is not num, float or bool", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = VAR;
    memcpy(&sv->as, value, elem_size(vtype));
    return sv;
}

SV* SVfromArray(const VType vtype, void* data, const int dim_count, int *dims) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == NUM_ARRAY || vtype == FLOAT_ARRAY || vtype == BOOL_ARRAY),
        "Array type (%i) is not num, float or bool", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = VAR;
    sv->as.array.data = data;
    sv->as.array.dim_count = dim_count;
    sv->as.array.dims = dims;
    return sv;
}

void SVdestroy(SV** sv) {
#ifdef DEBUGGING
    ASSERT_MSG((sv != NULL && *sv != NULL), "Invalid double pointer to SV in SVdestroy");
#endif // DEBUGGING
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
