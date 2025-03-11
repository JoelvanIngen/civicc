//src/variables/variable.h

#pragma once

#include "common.h"

#include "../analysis/argstack.h"

typedef enum {
    SV_VT_NUM,
    SV_VT_FLOAT,
    SV_VT_BOOL,
    SV_VT_VOID,
    SV_VT_NUM_ARRAY,
    SV_VT_FLOAT_ARRAY,
    SV_VT_BOOL_ARRAY,
} VType;                            // ValueType

typedef enum {
    SV_NT_FN,
    SV_NT_VAR,
} NType;                            // NodeType

typedef struct {
    VType data_t;                   // Data type as defined in ccngen/enum.h
    NType node_t;                   // Node type as defined in ccngen/enum.h
    union {                         // Extra data for arrays and functions
        struct {                    // Struct for array var type, or parameters for function type
            void* data;             // Pointer to start of array data / function parameter types
            size_t dim_count;       // Amount of array dimensions, or amount of function params
            size_t* dims;           // Dimension sizes of array
        } array;
        ParamList function;
    } as;                           // Using `as` as name for union because it reads nicely
} ScopeValue;

// Type access
#define GET_TYPE(v)         ((v)->data_t)

// Type checking macros
#define IS_NUM(v)           ((v)->data_t == VType::SV_VT_NUM)
#define IS_FLOAT(v)         ((v)->data_t == VType::SV_VT_FLOAT)
#define IS_BOOL(v)          ((v)->data_t == VType::SV_VT_BOOL)
#define IS_VOID(v)          ((v)->data_t == VType::SV_VT_VOID)
#define IS_NUM_ARRAY(v)     ((v)->data_t == VType::SV_VT_NUM_ARRAY)
#define IS_FLOAT_ARRAY(v)   ((v)->data_t == VType::SV_VT_FLOAT_ARRAY)
#define IS_BOOL_ARRAY(v)    ((v)->data_t == VType::SV_VT_BOOL_ARRAY)
#define IS_ARRAY(v) \
    (IS_NUM_ARRAY(v) || IS_FLOAT_ARRAY(v) || IS_BOOL_ARRAY(v))

#define HAS_SAME_TYPE(v1, v2) ((v1)->data_t == (v2)->data_t)

// Access macros
#ifdef DEBUGGING
#define AS_NUM(v)           (assert(IS_NUM(v)), (v)->as.num_val)
#define AS_FLOAT(v)         (assert(IS_FLOAT(v)), (v)->as.float_val)
#define AS_BOOL(v)          (assert(IS_BOOL(v)), (v)->as.bool_val)
#define AS_NUM_ARRAY(v)     (assert(IS_NUM_ARRAY(v)), (int64_t *)((v)->as.array.data))
#define AS_FLOAT_ARRAY(v)   (assert(IS_FLOAT_ARRAY(v)), (double *)((v)->as.array.data))
#define AS_BOOL_ARRAY(v)    (assert(IS_BOOL_ARRAY(v)), (bool *)((v)->as.array.data))
#else // DEBUGGING
#define AS_NUM(v)           ((v)->as.num_val)
#define AS_FLOAT(v)         ((v)->as.float_val)
#define AS_BOOL(v)          ((v)->as.bool_val)
#define AS_NUM_ARRAY(v)     ((int64_t *)((v)->as.array.data))
#define AS_FLOAT_ARRAY(v)   ((double *)((v)->as.array.data))
#define AS_BOOL_ARRAY(v)    ((bool *)((v)->as.array.data))
#endif // DEBUGGING

ScopeValue* SVfromFun(VType vtype);
ScopeValue* SVfromVar(VType vtype, const void* value);
ScopeValue* SVfromArray(VType vtype, void* data, int dim_count, int *dims);
void SVdestroy(ScopeValue** sv);
