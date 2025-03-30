// src/bytecode/asm.c

#include "asm.h"

/**
 * Initialises an existing assembly struct
 * @param assembly assembly struct to initialise
 */
void ASMinit(Assembly* assembly) {
    assembly->instrs = NULL;
    assembly->last_instr = NULL;
    assembly->init_instrs = NULL;
    assembly->last_init_instr = NULL;
    assembly->fun_exports = NULL;
    assembly->last_fun_export = NULL;
    assembly->var_exports = NULL;
    assembly->last_var_export = NULL;
    assembly->glob_vars = NULL;
    assembly->last_glob_var = NULL;
    assembly->fun_imports = NULL;
    assembly->last_fun_import = NULL;
    assembly->var_imports = NULL;
    assembly->last_var_import = NULL;
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

static Instruction* new_init_instruction(Assembly* assembly) {
    Instruction* instr = MEMmalloc(sizeof(Instruction));
    instr->next = NULL;
    if (assembly->last_init_instr == NULL) {
        assembly->init_instrs = instr;
    } else {
        assembly->last_init_instr->next = instr;
    }

    assembly->last_init_instr = instr;

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
    if (assembly->last_fun_export == NULL) assembly->fun_exports = fun_export;
    else assembly->last_fun_export->next = fun_export;

    assembly->last_fun_export = fun_export;

    return fun_export;
}

static void free_fun_export(FunExport* fun_export) {
    MEMfree(fun_export->name);
    MEMfree(fun_export->ret_type);
    // TODO: Free args once we figure out the data structure to use
    MEMfree(fun_export);
}

static VarExport* new_var_export(Assembly* assembly) {
    VarExport* var_export = MEMmalloc(sizeof(VarExport));
    var_export->next = NULL;

    if (assembly->var_exports == NULL) assembly->var_exports = var_export;
    else assembly->last_var_export->next = var_export;

    assembly->last_var_export = var_export;

    return var_export;
}

static void free_var_export(VarExport* var_export) {
    MEMfree(var_export->name);
    MEMfree(var_export);
}

static GlobVar* new_globvar(Assembly* assembly) {
    GlobVar* globvar = MEMmalloc(sizeof(GlobVar));
    globvar->next = NULL;

    if (assembly->glob_vars == NULL) assembly->glob_vars = globvar;
    else assembly->last_glob_var->next = globvar;

    assembly->last_glob_var = globvar;

    return globvar;
}

static void free_globvar(GlobVar* globvar) {
    MEMfree(globvar->type);
    MEMfree(globvar);
}

static FunImport* new_fun_import(Assembly* assembly) {
    FunImport* fun_import = MEMmalloc(sizeof(FunImport));
    fun_import->next = NULL;

    if (assembly->fun_imports == NULL) assembly->fun_imports = fun_import;
    else assembly->last_fun_import->next = fun_import;

    assembly->last_fun_import = fun_import;

    return fun_import;
}

static void free_fun_import(FunImport* fun_import) {
    MEMfree(fun_import->name);
    MEMfree(fun_import->ret_type);
    // TODO: Free args once we figure out the data structure to use
    MEMfree(fun_import);
}

static VarImport* new_var_import(Assembly* assembly) {
    VarImport* var_import = MEMmalloc(sizeof(VarImport));
    var_import->next = NULL;

    if (assembly->var_imports == NULL) assembly->var_imports = var_import;
    else assembly->last_var_import->next = var_import;

    assembly->last_var_import = var_import;

    return var_import;
}

static void free_var_import(VarImport* var_import) {
    MEMfree(var_import->name);
    MEMfree(var_import->type);
    MEMfree(var_import);
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

    // Free init instructions
    Instruction* init_inst = assembly->init_instrs;
    while (init_inst != NULL) {
        free_instruction(init_inst);
        init_inst = init_inst->next;
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

    // Free var exports
    VarExport* var_export = assembly->var_exports;
    while (var_export != NULL) {
        free_var_export(var_export);
        var_export = var_export->next;
    }

    // Free globvars
    GlobVar* globvar = assembly->glob_vars;
    while (globvar != NULL) {
        free_globvar(globvar);
        globvar = globvar->next;
    }

    // Free fun imports
    FunImport* fun_import = assembly->fun_imports;
    while (fun_import != NULL) {
        free_fun_import(fun_import);
        fun_import = fun_import->next;
    }

    // Free var imports
    VarImport* var_import = assembly->var_imports;
    while (var_import != NULL) {
        free_var_import(var_import);
        var_import = var_import->next;
    }

    ASMinit(assembly);
    *assembly_ptr = NULL;
}

void ASMemitInstr(Assembly* assembly, char* instr_name, char* arg0, char* arg1, char* arg2) {
    Instruction* instr = new_instruction(assembly);
    instr->instr = STRcpy(instr_name);
    instr->is_label = false;
    instr->is_fun = false;
    instr->arg0 = arg0 ? STRcpy(arg0) : NULL;
    instr->arg1 = arg1 ? STRcpy(arg1) : NULL;
    instr->arg2 = arg2 ? STRcpy(arg2) : NULL;
}

void ASMemitInit(Assembly* assembly, char* instr_name, char* arg0, char* arg1, char* arg2) {
    Instruction* instr = new_init_instruction(assembly);
    instr->instr = STRcpy(instr_name);
    instr->is_label = false;
    instr->is_fun = false;
    instr->arg0 = arg0 ? STRcpy(arg0) : NULL;
    instr->arg1 = arg1 ? STRcpy(arg1) : NULL;
    instr->arg2 = arg2 ? STRcpy(arg2) : NULL;
}

void ASMemitLabel(Assembly* assembly, char* label, bool is_fun) {
    Instruction* instr = new_instruction(assembly);
    instr->instr = STRcpy(label);
    instr->is_label = true;
    instr->is_fun = is_fun;
}

void ASMemitConst(Assembly* assembly, char* type, char* val) {
    Constant* constant = new_constant(assembly);
    constant->type = type;
    constant->value = STRcpy(val);
}

void ASMemitFunExport(Assembly* assembly, char* name, char* ret_type, size_t arglen, char** args) {
    FunExport* fun_export = new_fun_export(assembly);
    fun_export->name = STRcpy(name);
    fun_export->ret_type = STRcpy(ret_type);
    fun_export->arg_amount = arglen;
    fun_export->args = args;
}

void ASMemitVarExport(Assembly* assembly, char* name, size_t glob_index) {
    VarExport* var_export = new_var_export(assembly);
    var_export->name = STRcpy(name);
    var_export->global_index = glob_index;
}

void ASMemitGlobVar(Assembly* assembly, char* type) {
    GlobVar* globvar = new_globvar(assembly);
    globvar->type = STRcpy(type);
}

void ASMemitFunImport(Assembly* assembly, char* name, char* ret_type, size_t arg_amount, char** args) {
    FunImport* fun_import = new_fun_import(assembly);
    fun_import->name = STRcpy(name);
    fun_import->ret_type = STRcpy(ret_type);
    fun_import->arg_amount = arg_amount;
    fun_import->args = args;
}

void ASMemitVarImport(Assembly* assembly, char* name, char* type) {
    VarImport* var_import = new_var_import(assembly);
    var_import->name = STRcpy(name);
    var_import->type = STRcpy(type);
}

ConstEntry ASMfindConstant(const Assembly* assembly, const char* value) {
    size_t idx = 0;
    Constant* constant = assembly->consts;
    while (constant != NULL) {
        if (strcmp(constant->value, value) == 0) {
            return (ConstEntry){idx, constant};
        }

        constant = constant->next;
        idx++;
    }

    // Not found
    return (ConstEntry){idx, NULL};
}

FunExportEntry ASMfindFunExport(const Assembly* assembly, const char* name) {
    size_t idx = 0;
    FunExport* export = assembly->fun_exports;
    while (export != NULL) {
        if (strcmp(export->name, name) == 0) {
            return (FunExportEntry){idx, export};
        }

        export = export->next;
        idx++;
    }

    // Not found
    return (FunExportEntry){0, NULL};
}

FunImportEntry ASMfindFunImport(const Assembly* assembly, const char* name) {
    size_t idx = 0;
    FunImport* import = assembly->fun_imports;
    while (import != NULL) {
        if (strcmp(import->name, name) == 0) {
            return (FunImportEntry){idx, import};
        }

        import = import->next;
        idx++;
    }

    // Not found
    return (FunImportEntry){0, NULL};
}
