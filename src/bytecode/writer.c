// src/bytecode/writer.c

#include "writer.h"

bool WRITTEN_FIRST_LABEL = false;

static void write_single_instruction(FILE* f, const Instruction* instruction) {
    if (instruction->is_label) {
        // Write extra newline, but not if this is the first label
        if (WRITTEN_FIRST_LABEL) fprintf(f, "\n");
        else WRITTEN_FIRST_LABEL = true;

        // Write name followed by colon
        fprintf(f, "%s:", instruction->instr);
    } else {
        // Write tab and instruction
        fprintf(f, "\t%s", instruction->instr);
        if (instruction->arg0 != NULL) fprintf(f, " %s", instruction->arg0); else return;
        if (instruction->arg1 != NULL) fprintf(f, " %s", instruction->arg1); else return;
        if (instruction->arg2 != NULL) fprintf(f, " %s", instruction->arg2);
    }
}

/**
 * Traverses linked list and calls writer for each instruction
 */
static void write_instructions(FILE* f, const Instruction* instruction) {
    while (instruction != NULL) {
        write_single_instruction(f, instruction);
        fprintf(f, "\n");
        instruction = instruction->next;
    }
}

void write_assembly(FILE* f, const Assembly* ASM) {
    write_instructions(f, ASM->instrs);
    fprintf(f, "\n");  // Extra newline like in examples
    // write_glob_vartable(f, ASM->glob_vars);
    // write_const_table(f, ASM->consts);
    // write_fun_import_table(f, ASM->fun_imports);
    // write_fun_export_table(f, ASM->fun_exports);
    // write_var_import_table(f, ASM->var_imports);
    // write_var_export_table(f, ASM->var_exports);
}
