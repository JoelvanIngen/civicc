//src/variables/variable.c

#include "variable.h"

#include "palm/hash_table.h"
#include "palm/memory.h"

typedef ScopeValue SV;

static size_t elem_size(const VType type) {
    switch (type) {
        case SV_VT_NUM:
        case SV_VT_NUM_ARRAY: return sizeof(int64_t);
        case SV_VT_FLOAT:
        case SV_VT_FLOAT_ARRAY: return sizeof(double);
        case SV_VT_BOOL:
        case SV_VT_BOOL_ARRAY: return sizeof(bool);
        case SV_VT_VOID: return 0;
        default: return -1; // TODO: ERROR
    }
}

static SV* SVcreate() {
    return MEMmalloc(sizeof(SV));
}

SV* SVfromFun(const VType vtype, VType* params, size_t params_len) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == SV_VT_NUM || vtype == SV_VT_FLOAT || vtype == SV_VT_BOOL || vtype == SV_VT_VOID),
        "Function return type (%i) is not num, float, bool or void", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = SV_NT_FN;
    sv->as.array.data = params;
    sv->as.array.dim_count = params_len;
    return sv;
}

SV* SVfromVar(const VType vtype, const void* value) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == SV_VT_NUM || vtype == SV_VT_FLOAT || vtype == SV_VT_BOOL),
        "Var type (%i) is not num, float or bool", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = SV_NT_VAR;
    memcpy(&sv->as, value, elem_size(vtype));
    return sv;
}

SV* SVfromArray(const VType vtype, void* data, const int dim_count, int *dims) {
#ifdef DEBUGGING
    ASSERT_MSG((vtype == SV_VT_NUM_ARRAY || vtype == SV_VT_FLOAT_ARRAY || vtype == SV_VT_BOOL_ARRAY),
        "Array type (%i) is not num, float or bool", vtype);
#endif // DEBUGGING
    ScopeValue* sv = SVcreate();
    sv->data_t = vtype;
    sv->node_t = SV_NT_VAR;
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
        case SV_VT_NUM_ARRAY:
        case SV_VT_FLOAT_ARRAY:
        case SV_VT_BOOL_ARRAY:
            MEMfree((*sv)->as.array.data);
            MEMfree((*sv)->as.array.dims);
            break;
        default:
            break;
    }

    MEMfree(*sv);
    *sv = NULL;
}
