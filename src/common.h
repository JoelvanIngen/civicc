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

#define VARTABLE_STACK_SIZE 10
#define VARTABLE_SIZE 100
#define INITIAL_LIST_SIZE 5

#define DEBUGGING true

/* Error with debug information for developing civic */
#define ERROR(fmt, ...) do { \
    fprintf(stderr, "RUNTIME ERROR: "); \
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
    exit(1); \
} while (false)

#define ARRAY_RESIZE(arr, new_size) (arr = MEMrealloc(arr, (new_size) * sizeof(*(arr))))
