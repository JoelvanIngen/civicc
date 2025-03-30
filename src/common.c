// src/common.c

#include "common.h"

#include "ccngen/enum.h"

char* ct_to_str(const enum Type t) {
    switch (t) {
        case CT_int: return "int";
        case CT_float: return "float";
        case CT_bool: return "bool";
        case CT_void: return "void";
        default: return "UNKNOWN";
    }
}

char* vt_to_str(const ValueType vt) {
    switch (vt) {
        case VT_NUM: return "int";
        case VT_FLOAT: return "float";
        case VT_BOOL: return "bool";
        case VT_VOID: return "void";
        case VT_NUMARRAY: return "int[]";
        case VT_FLOATARRAY: return "float[]";
        case VT_BOOLARRAY: return "bool[]";
        default: return "UNKNOWN";
    }
}

ValueType ct_to_vt(const enum Type ct_type, const bool is_array) {
    switch (ct_type) {
        case CT_int: return is_array ? VT_NUMARRAY : VT_NUM;
        case CT_float: return is_array ? VT_FLOATARRAY : VT_FLOAT;
        case CT_bool: return is_array ? VT_BOOLARRAY : VT_BOOL;
        case CT_void:
            if (is_array) {
                // Doesn't exist
                USER_ERROR("Type error: tried to initialise void array");
                // HAD_ERROR = true;
            }
        return VT_VOID;
        default:
#ifdef DEBUGGING
            ERROR("Unexpected CT_TYPE %s", ct_to_str(ct_type));
#endif // DEBUGGING
    }
}

char* bo_to_str(const enum BinOpType op) {
    switch (op) {
        case BO_add: return "BINOP_ADD";
        case BO_sub: return "BINOP_SUB";
        case BO_mul: return "BINOP_MUL";
        case BO_div: return "BINOP_DIV";
        case BO_mod: return "BINOP_MOD";
        case BO_lt: return "BINOP_LT";
        case BO_le: return "BINOP_LE";
        case BO_gt: return "BINOP_GT";
        case BO_ge: return "BINOP_GE";
        case BO_eq: return "BINOP_EQ";
        case BO_ne: return "BINOP_NE";
        case BO_and: return "BINOP_AND";
        case BO_or: return "BINOP_OR";
        default: return "BINOP_UNKNOWN";
    }
}

char* mo_to_str(const enum MonOpType op) {
    switch (op) {
        case MO_neg: return "MONOP_NEG";
        case MO_not: return "MONOP_NOT";
        default: return "MONOP_UNKNOWN";
    }
}

char* float_to_str(const float f) {
    char* buf = MEMmalloc(MAX_STR_LEN);
    snprintf(buf, MAX_STR_LEN, "%f", f);
    return buf;
}

char* int_to_str(const int i) {
    char* buf = MEMmalloc(MAX_STR_LEN);
    snprintf(buf, MAX_STR_LEN, "%i", i);
    return buf;
}

ValueType demote_array_type(const ValueType array_type) {
    switch (array_type) {
        case VT_NUMARRAY: return VT_NUM;
        case VT_FLOATARRAY: return VT_FLOAT;
        case VT_BOOLARRAY: return VT_BOOL;
        default: // Should never occur
#ifdef DEBUGGING
            ERROR("Cannot demote ValueType array %i", array_type);
#endif
    }
}

/**
 * Concatenates two strings and frees them
 * @param s1 first string
 * @param s2 second string
 * @return concatenated strings
 */
char* safe_concat_str(char* s1, char* s2) {
    char* buf = MEMmalloc(MAX_STR_LEN);
    snprintf(buf, MAX_STR_LEN, "%s%s", s1, s2);
    MEMfree(s1);
    MEMfree(s2);
    return buf;
}

char* generate_array_dim_name(const char* parent_name, const size_t i) {
    return safe_concat_str(
            safe_concat_str(
                safe_concat_str(STRcpy("_index"),
                    int_to_str((int) i)),
                    STRcpy("_")),
                STRcpy(parent_name));
}
