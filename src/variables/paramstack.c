// src/variables/paramstack.c

#include "paramstack.h"

#include "common.h"

#define INITIAL_SIZE 10

typedef ParamListStack PLS;
typedef ParamList PL;

static void checkResizePL(PL* pl) {
    if (pl->ptr + 1 >= pl->size) {
        pl->size = pl->size * 2;
        pl->args = MEMrealloc(pl->args, pl->size * sizeof(Parameter));
    }
}

static void checkResizePLS(PLS* pls) {
    if (pls->ptr + 1 >= pls->size) {
        pls->size = pls->size * 2;
        pls->fun_calls = MEMrealloc(pls->fun_calls, pls->size * sizeof(Parameter));
    }
}

static PL* PLinit(PL* pl) {
    pl->ptr = 0;
    pl->size = INITIAL_SIZE;
    pl->args = MEMmalloc(pl->size * sizeof(Parameter));
    return pl;
}

static void PLfree(PL* pl) {
    free(pl->args);
    free(pl);
}

void PLadd(PL* pl, const char* name, const VType type) {
    checkResizePL(pl);
    pl->args[pl->ptr].type = type;
    pl->args[pl->ptr].name = name;
    pl->ptr++;
}

/** Creates a new parameter list stack */
PLS* PLSnew() {
    PLS* pls = MEMmalloc(sizeof(PLS));
    pls->ptr = 0;
    pls->size = INITIAL_SIZE;
    pls->fun_calls = MEMmalloc(pls->size * sizeof(PL));
    return pls;
}

/** Frees a parameter list stack */
void PLSfree(PLS** pls) {
    // Free all internal lists
    for (size_t i = 0; i < (*pls)->ptr; i++) {
        PLfree(&(*pls)->fun_calls[i]);
    }

    // Free self
    MEMfree(*pls);
    *pls = NULL;
}

/** Starts a new funcall */
void PLSpush(PLS* pls) {
    checkResizePLS(pls);
    PLinit(&pls->fun_calls[pls->ptr]);
    pls->ptr++;
}

/** Ends a funcall and discards list */
void PLSpop(PLS* pls) {
    pls->ptr--;
    PLfree(&pls->fun_calls[pls->ptr]);
}

/** Adds a new parameter on the current funcall */
void PLSadd(PLS* pls, const char* name, const VType type) {
    PLadd(&pls->fun_calls[pls->ptr - 1], name, type);
}

/** Points to the first argument of the current scope */
Parameter* PLSgetCurrentArgs(const PLS* pls) {
    return pls->fun_calls[pls->ptr - 1].args;
}

/** Returns the length of the current scope (upper bound exclusive) */
size_t PLSgetCurrentLength(const PLS* pls) {
    return pls->fun_calls[pls->ptr - 1].ptr;
}
