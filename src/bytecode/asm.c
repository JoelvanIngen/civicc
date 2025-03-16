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
        assembly->last_instr = instr;
    } else {
        assembly->last_instr->next = instr;
        assembly->last_instr = instr;
    }
    return instr;
}

static void free_instruction(Instruction* instr) {
    MEMfree(instr->instr);
    MEMfree(instr->arg0);
    MEMfree(instr->arg1);
    MEMfree(instr->arg2);
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
