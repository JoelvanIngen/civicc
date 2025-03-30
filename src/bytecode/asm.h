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
    char* name;                 // Name of exported function, also label name
    char* ret_type;             // Return type of function
    size_t arg_amount;
    char** args;                // Argument types
    struct FunExport* next;
} FunExport;

typedef struct VarExport {
    char* name;
    size_t global_index;        // Index of var at global table
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
    Instruction* init_instrs;
    Instruction* last_init_instr;
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

typedef struct ConstEntry {             // Used for finding and retrieving values already written to ASM
    size_t offset;                      // Offset in final written ASM
    Constant* get;                      // The result itself
} ConstEntry;

typedef struct FunExportEntry {
    size_t offset;
    FunExport* get;
} FunExportEntry;

typedef struct FunImportEntry {
    size_t offset;
    FunImport* get;
} FunImportEntry;

void ASMinit(Assembly* assembly);
void ASMfree(Assembly** assembly_ptr);

void ASMemitInstr(Assembly* assembly, const char* instr_name, const char* arg0, const char* arg1, const char* arg2);
void ASMemitInit(Assembly* assembly, const char* instr_name, const char* arg0, const char* arg1, const char* arg2);
void ASMemitLabel(Assembly* assembly, const char* label, bool is_fun);
void ASMemitConst(Assembly* assembly, char* type, const char* val);
void ASMemitFunExport(Assembly* assembly, const char* name, const char* ret_type, size_t arglen, char** args);
void ASMemitVarExport(Assembly* assembly, char* name, size_t glob_index);
void ASMemitGlobVar(Assembly* assembly, char* type);
void ASMemitFunImport(Assembly* assembly, char* name, char* ret_type, size_t arg_amount, char** args);
void ASMemitVarImport(Assembly* assembly, char* name, char* type);

ConstEntry ASMfindConstant(const Assembly* assembly, const char* value);
FunExportEntry ASMfindFunExport(const Assembly* assembly, const char* name);
FunImportEntry ASMfindFunImport(const Assembly* assembly, const char* name);
