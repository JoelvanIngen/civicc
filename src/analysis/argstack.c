// src/variables/paramstack.c

#include "argstack.h"

#include "common.h"

#define INITIAL_SIZE 10

typedef ArgListStack ALS;
typedef ArgList AL;

static void checkResizeAL(AL* al) {
    if (al->ptr + 1 >= al->capacity) {
        al->capacity = al->capacity * 2;
        al->args = MEMrealloc(al->args, al->capacity * sizeof(Argument));
    }
}

static void checkResizeALS(ALS* als) {
    if (als->ptr + 1 >= als->capacity) {
        als->capacity = als->capacity * 2;
        als->fun_calls = MEMrealloc(als->fun_calls, als->capacity * sizeof(Argument));
    }
}

static AL* ALnew() {
    AL* al = MEMmalloc(sizeof(ArgList));
    al->ptr = 0;
    al->capacity = INITIAL_SIZE;
    al->args = MEMmalloc(al->capacity * sizeof(Argument));
    return al;
}

static void ALfree(AL* al) {
    for (size_t i = 0; i < al->ptr; i++) {
        MEMfree(al->args[i].name);
    }

    free(al->args);
    free(al);
}

void ALadd(AL* al, char* name, const ValueType type) {
    checkResizeAL(al);
    al->args[al->ptr].type = type;
    al->args[al->ptr].name = STRcpy(name);
    al->ptr++;
}

/** Creates a new parameter list stack */
ALS* ALSnew() {
    ALS* als = MEMmalloc(sizeof(ALS));
    als->ptr = 0;
    als->capacity = INITIAL_SIZE;
    als->fun_calls = MEMmalloc(als->capacity * sizeof(AL));
    return als;
}

/** Frees a parameter list stack */
void ALSfree(ALS** als) {
    // Free all internal lists
    for (size_t i = 0; i < (*als)->ptr; i++) {
        ALfree((*als)->fun_calls[i]);
    }

    // Free self
    MEMfree(*als);
    *als = NULL;
}

/** Starts a new funcall */
void ALSpush(ALS* als) {
    checkResizeALS(als);
    als->fun_calls[als->ptr] = ALnew();
    als->ptr++;
}

/** Ends a funcall and discards list */
void ALSpop(ALS* als) {
    als->ptr--;
    ALfree(als->fun_calls[als->ptr]);
}

/** Adds a new parameter on the current funcall */
void ALSadd(ALS* als, char* name, const ValueType type) {
    ALadd(als->fun_calls[als->ptr - 1], name, type);
}

/** Points to the first argument of the current scope */
Argument** ALSgetCurrentArgs(const ALS* als) {
    return &als->fun_calls[als->ptr - 1]->args;
}

/** Returns the length of the current scope (upper bound exclusive) */
size_t ALSgetCurrentLength(const ALS* als) {
    return als->fun_calls[als->ptr - 1]->ptr;
}
