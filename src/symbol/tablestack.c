// src/symbol/tablestack.c

#include "tablestack.h"

typedef SymbolTableStack STS;

static SymbolTable* top(const STS* sts) {
    return sts->tables[sts->ptr - 1];
}

static SymbolTable* peek(const STS* sts, const size_t idx) {
    return sts->tables[idx];
}

void STSpop(STS* sts) {
    sts->ptr--;
    STfree(&sts->tables[sts->ptr]);
    sts->tables[sts->ptr] = NULL;
}

STS* STSnew() {
    STS* sts = MEMmalloc(sizeof(STS));
    sts->ptr = 0;
    sts->capacity = VARTABLE_STACK_SIZE;
    sts->tables = MEMmalloc(VARTABLE_STACK_SIZE * sizeof(SymbolTable*));
    return sts;
}

void STSfree(STS** sts_ptr) {
    STS* sts = *sts_ptr;

    while (sts->ptr > 0) {
        STSpop(sts);
    }

    sts->ptr = 0;
    sts->capacity = 0;
    MEMfree(sts->tables);
    MEMfree(sts);
    *sts_ptr = NULL;
}

void STSpush(STS* sts, char* scope_name, const ValueType ret_type) {
    if (sts->ptr >= sts->capacity) {
        sts->capacity *= 2;
        sts->tables = MEMrealloc(sts->tables, sts->capacity * sizeof(SymbolTable*));
    }

    sts->tables[sts->ptr] = STnew(scope_name, ret_type);
    sts->ptr++;
}

Symbol* STSlookup(const STS* sts, char* symbol_name) {
    for (long height = (long) sts->ptr - 1; height >= 0; height--) {
        const SymbolTable* st = peek(sts, height);
        Symbol* s = STlookup(st, symbol_name);
        if (s != NULL) {
            return s;
        }
    }

    return NULL;
}

void STSadd(const STS* sts, char* symbol_name, Symbol* sym) {
    if (STlookup(top(sts), symbol_name) != NULL) {
        USER_ERROR("Symbol %s already exists", symbol_name);
    }

    STinsert(top(sts), symbol_name, sym);
}

char* STScurrentScopeName(const STS* sts) {
    return top(sts)->name;
}
