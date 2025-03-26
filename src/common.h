// src/common.h

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"

#include "ccngen/enum.h"

#include "types/types.h"

#define VARTABLE_STACK_SIZE 10
#define VARTABLE_SIZE 100
#define INITIAL_LIST_SIZE 5
#define MAX_STR_LEN 100

#define DEBUGGING true

/* Error with debug information for developing civic */
#define ERROR(fmt, ...) do { \
    fprintf(stderr, "DEBUG ERROR AT %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
    assert(false); \
} while (false)

#define ASSERT_MSG(cond, fmt, ...) do { \
    if (!cond) { ERROR(fmt, ##__VA_ARGS__); } \
} while (false)

/* Error for developers using civic */
#define USER_ERROR(fmt, ...) do { \
    fprintf(stderr, "ERROR: "); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
} while (false)

#define ARRAY_RESIZE(arr, new_size) (arr = MEMrealloc(arr, (new_size) * sizeof(*(arr))))

char* ct_to_str(enum Type t);
char* vt_to_str(ValueType vt);
ValueType ct_to_vt(enum Type ct_type, bool is_array);
char* bo_to_str(enum BinOpType op);
char* mo_to_str(enum MonOpType op);
char* safe_concat_str(char* s1, char* s2);
