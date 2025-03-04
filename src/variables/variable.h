//src/analysis/variable.h

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum ccn_nodetype;

typedef enum {
    NUM,
    FLOAT,
    BOOL,
    VOID,
    NUM_ARRAY,
    FLOAT_ARRAY,
    BOOL_ARRAY,
} DType;                            // DType to prevent name collision with enum Type

typedef struct {
    DType data_t;                   // Data type as defined in ccngen/enum.h
    enum ccn_nodetype node_t;       // Node type as defined in ccngen/enum.h
    union {                         // The actual data
        int64_t num_val;
        double float_val;
        bool bool_val;
        struct {                    // Struct for array type
            void* data;             // Pointer to start of array data
            size_t elem_size;       // Size of single element, might actually be unneeded but we will see later
            int dim_count;          // Amount of dimensions
            int* dims;              // Dimension sizes
        } array;
    } as;                           // Using `as` as name for union because it reads nicely
} ScopeValue;

#define IS_NUM(value)       ((value)->data_t == DType::NUM)
#define IS_FLOAT(value)     ((value)->data_t == DType::FLOAT)
#define IS_BOOL(value)      ((value)->data_t == DType::BOOL)
#define IS_VOID(value)      ((value)->data_t == DType::VOID)
#define IS_NUM_ARRAY(value)     ((value)->data_t == DType::NUM_ARRAY)
#define IS_FLOAT_ARRAY(value)   ((value)->data_t == DType::FLOAT_ARRAY)
#define IS_BOOL_ARRAY(value)    ((value)->data_t == DType::BOOL_ARRAY)

#define IS_ARRAY(value) \
    (IS_NUM_ARRAY(value) || IS_FLOAT_ARRAY(value) || IS_BOOL_ARRAY(value))

#define AS_NUM(value)       ((value)->as.num_val)
#define AS_FLOAT(value)     ((value)->as.float_val)
#define AS_BOOL(value)      ((value)->as.bool_val)
#define AS_NUM_ARRAY(value)     (int64_t *)((value)->as.array.data)
#define AS_FLOAT_ARRAY(value)   ((double *)((value)->as.array.data))
#define AS_BOOL_ARRAY(value)    ((bool *)((value)->as.array.data))

ScopeValue* TEcreate();
void TEdestroy(ScopeValue** sv);
