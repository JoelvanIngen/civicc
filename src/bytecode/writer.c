// src/bytecode/writer.c

#include "writer.h"

bool WRITTEN_FIRST_LABEL = false;

static void write_single_instruction(FILE* f, const Instruction* instruction) {
    if (instruction->is_label) {
        // Write extra newline for functions, but not if this is the first label
        if (WRITTEN_FIRST_LABEL && instruction->is_fun) fprintf(f, "\n");
        else WRITTEN_FIRST_LABEL = true;

        // Write name followed by colon
        fprintf(f, "%s:", instruction->instr);
    } else {
        // Write tab and instruction
        fprintf(f, "    %s", instruction->instr);
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

static void write_single_constant(FILE* f, const Constant* constant) {
    fprintf(f, ".const %s %s", constant->type, constant->value);
}

static void write_constants(FILE* f, const Constant* constant) {
    while (constant != NULL) {
        write_single_constant(f, constant);
        fprintf(f, "\n");
        constant = constant->next;
    }
}

static void write_single_fun_export(FILE* f, const FunExport* export) {
    fprintf(f, ".exportfun \"%s\"", export->name);

    fprintf(f, " %s", export->ret_type);

    for (size_t i = 0; i < export->arg_amount; i++) {
        fprintf(f, " %s", export->args[i]);
    }

    fprintf(f, " %s", export->name);
}

static void write_fun_exports(FILE* f, const FunExport* export) {
    while (export != NULL) {
        write_single_fun_export(f, export);
        fprintf(f, "\n");
        export = export->next;
    }
}

static void write_single_var_export(FILE* f, const VarExport* export) {
    fprintf(f, ".exportvar \"%s\" %lu", export->name, export->global_index);
}

static void write_var_exports(FILE* f, const VarExport* export) {
    while (export != NULL) {
        write_single_var_export(f, export);
        fprintf(f, "\n");
        export = export->next;
    }
}

static void write_single_globvar(FILE* f, const GlobVar* globvar) {
    fprintf(f, ".global %s", globvar->type);
}

static void write_globvars(FILE* f, const GlobVar* globvar) {
    while (globvar != NULL) {
        write_single_globvar(f, globvar);
        fprintf(f, "\n");
        globvar = globvar->next;
    }
}

static void write_single_fun_import(FILE* f, const FunImport* import) {
    fprintf(f, ".importfun \"%s\" %s", import->name, import->ret_type);

    for (size_t i = 0; i < import->arg_amount; i++) {
        fprintf(f, " %s", import->args[i]);
    }
}

static void write_fun_imports(FILE* f, const FunImport* import) {
    while (import != NULL) {
        write_single_fun_import(f, import);
        fprintf(f, "\n");
        import = import->next;
    }
}

static void write_single_var_import(FILE* f, const VarImport* import) {
    fprintf(f, ".importvar \"%s\" %s", import->name, import->type);
}

static void write_var_imports(FILE* f, const VarImport* import) {
    while (import != NULL) {
        write_single_var_import(f, import);
        fprintf(f, "\n");
        import = import->next;
    }
}

void write_assembly(FILE* f, const Assembly* ASM) {
    write_instructions(f, ASM->instrs);
    fprintf(f, "\n");  // Extra newline like in examples
    write_constants(f, ASM->consts);
    write_fun_exports(f, ASM->fun_exports);
    write_var_exports(f, ASM->var_exports);
    write_globvars(f, ASM->glob_vars);
    write_fun_imports(f, ASM->fun_imports);
    write_var_imports(f, ASM->var_imports);
}
