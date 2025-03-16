// src/bytecode/asm.c

#include "asm.h"

/**
 * Initialises an existing assembly struct
 * @param assembly assembly struct to initialise
 */
void ASMinit(Assembly* assembly) {
    assembly->instrs = NULL;
    assembly->last_instr = NULL;
    assembly->fun_exports = NULL;
    assembly->last_fun_export = NULL;
}

static Instruction* new_instruction(Assembly* assembly) {
    Instruction* instr = MEMmalloc(sizeof(Instruction));
    instr->next = NULL;
    if (assembly->last_instr == NULL) {
        assembly->instrs = instr;
    } else {
        assembly->last_instr->next = instr;
    }

    assembly->last_instr = instr;

    return instr;
}

static void free_instruction(Instruction* instr) {
    MEMfree(instr->instr);
    MEMfree(instr->arg0);
    MEMfree(instr->arg1);
    MEMfree(instr->arg2);
}

static Constant* new_constant(Assembly* assembly) {
    Constant* constant = MEMmalloc(sizeof(Constant));
    constant->next = NULL;

    if (assembly->consts == NULL) assembly->consts = constant;
    else assembly->last_const->next = constant;

    assembly->last_const = constant;

    return constant;
}

static void free_constant(Constant* constant) {
    MEMfree(constant->type);
    MEMfree(constant->value);
}

static FunExport* new_fun_export(Assembly* assembly) {
    FunExport* fun_export = MEMmalloc(sizeof(FunExport));
    fun_export->next = NULL;
    if (assembly->last_fun_export == NULL) {
        assembly->fun_exports = fun_export;
    } else {
        assembly->last_fun_export->next = fun_export;
    }

    assembly->last_fun_export = fun_export;

    return fun_export;
}

static void free_fun_export(FunExport* fun_export) {
    MEMfree(fun_export->name);
    MEMfree(fun_export->type);
    // TODO: Free args once we figure out the data structure to use
}

/**
 * Frees an assembly struct
 * @param assembly_ptr pointer to assembly struct to free
 */
void ASMfree(Assembly** assembly_ptr) {
    Assembly* assembly = *assembly_ptr;

    // Free instructions
    Instruction* instr = assembly->instrs;
    while (instr != NULL) {
        free_instruction(instr);
        instr = instr->next;
    }

    // Free constants
    Constant* constant = assembly->consts;
    while (constant != NULL) {
        free_constant(constant);
        constant = constant->next;
    }

    // Free function exports
    FunExport* fun_export = assembly->fun_exports;
    while (fun_export != NULL) {
        free_fun_export(fun_export);
        fun_export = fun_export->next;
    }

    ASMinit(assembly);
    *assembly_ptr = NULL;
}

void ASMemitInstr(Assembly* assembly, char* instr_name, char* arg0, char* arg1, char* arg2, bool is_label) {
    Instruction* instr = new_instruction(assembly);
    strcpy(instr->instr, instr_name);
    instr->is_label = is_label;
    instr->arg0 = arg0 ? strdup(arg0) : NULL;
    instr->arg1 = arg1 ? strdup(arg1) : NULL;
    instr->arg2 = arg2 ? strdup(arg2) : NULL;
}

void ASMemitConst(Assembly* assembly, char* type, char* val) {
    Constant* constant = new_constant(assembly);
    constant->type = type;
    constant->value = val;
}

void ASMemitFunExport(Assembly* assembly, char* name, char* type, char** args, bool is_main) {
    FunExport* fun_export = new_fun_export(assembly);
    strcpy(fun_export->name, name);
    fun_export->is_main = is_main;
    strcpy(fun_export->type, type);
    fun_export->args = args;
}
