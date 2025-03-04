//src/variables/variable.h

#pragma once

#include "common.h"

typedef enum {
    NUM,
    FLOAT,
    BOOL,
    VOID,
    NUM_ARRAY,
    FLOAT_ARRAY,
    BOOL_ARRAY,
} VType;                            // ValueType

typedef enum {
    FN,
    VAR,
} NType;                            // NodeType

typedef struct {
    VType data_t;                   // Data type as defined in ccngen/enum.h
    NType node_t;                   // Node type as defined in ccngen/enum.h
    union {                         // The actual data
        int64_t num_val;
        double float_val;
        bool bool_val;
        struct {                    // Struct for array type
            void* data;             // Pointer to start of array data
            int dim_count;          // Amount of dimensions
            int* dims;              // Dimension sizes
        } array;
    } as;                           // Using `as` as name for union because it reads nicely
} ScopeValue;

// Type checking macros
#define IS_NUM(v)           ((v)->data_t == VType::NUM)
#define IS_FLOAT(v)         ((v)->data_t == VType::FLOAT)
#define IS_BOOL(v)          ((v)->data_t == VType::BOOL)
#define IS_VOID(v)          ((v)->data_t == VType::VOID)
#define IS_NUM_ARRAY(v)     ((v)->data_t == VType::NUM_ARRAY)
#define IS_FLOAT_ARRAY(v)   ((v)->data_t == VType::FLOAT_ARRAY)
#define IS_BOOL_ARRAY(v)    ((v)->data_t == VType::BOOL_ARRAY)
#define IS_ARRAY(v) \
    (IS_NUM_ARRAY(v) || IS_FLOAT_ARRAY(v) || IS_BOOL_ARRAY(v))

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
