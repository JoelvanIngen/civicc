// src/bytecode/asm.c

#pragma once

#include "common.h"

/** TODO: Add all necessary modules
 * - Instructions (done)
 * - Global variable table
 * - Constant table (done)
 * - Function import table
 * - Function export table (done)
 * - Variable import table
 * - Variable export table
 */

typedef struct Instruction {
    char* instr;                // Instruction string
    char* arg0, * arg1, * arg2; // Optional arguments for instruction
    bool is_label;              // Ugly hack to allow labels in the stream, instr will be label name
    bool is_fun;                // Function labels are prepended by a whitespace
    struct Instruction* next;   // We won't be changing any instructions once they're generated
                                // so with a linked list we avoid size checks
} Instruction;

typedef struct Constant {
    char* type;
    char* value;
    struct Constant* next;
} Constant;

typedef struct FunExport {
    char* name;                 // Name of exported function
    char* ret_type;             // Return type of function
    size_t arg_amount;
    char** args;                // Argument types
    char* label_name;           // Label name as labelled in instructions
    bool is_main;               // From example, it seems "main" should be appended if function is main?
    bool is_init;               // From example, it seems "__init" should be appended for some stuff?
    struct FunExport* next;
} FunExport;

typedef struct VarExport {
    char* name;
    size_t index;               // Index of var at global table
    struct VarExport* next;
} VarExport;

typedef struct GlobVar {
    char* type;
    struct GlobVar* next;
} GlobVar;

typedef struct FunImport {
    char* name;
    char* ret_type;
    size_t arg_amount;
    char** args;
    struct FunImport* next;
} FunImport;

typedef struct VarImport {
    char* name;
    char* type;
    struct VarImport* next;
} VarImport;

typedef struct {
    Instruction* instrs;
    Instruction* last_instr;
    Constant* consts;
    Constant* last_const;
    FunExport* fun_exports;
    FunExport* last_fun_export;
    VarExport* var_exports;
    VarExport* last_var_export;
    GlobVar* glob_vars;
    GlobVar* last_glob_var;
    FunImport* fun_imports;
    FunImport* last_fun_import;
    VarImport* var_imports;
    VarImport* last_var_import;
} Assembly;

void ASMinit(Assembly* assembly);
void ASMfree(Assembly** assembly_ptr);
void ASMemitInstr(Assembly* assembly, char* instr_name, char* arg0, char* arg1, char* arg2);
void ASMemitLabel(Assembly* assembly, char* label, bool is_fun);
void ASMemitConst(Assembly* assembly, char* type, char* val);
void ASMemitFunExport(Assembly* assembly, char* name, char* type, char** args, bool is_main);
