// src/bytecode/asm.c

#pragma once

#include "common.h"

/** TODO: Add all necessary modules
 * - Instructions (done)
 * - Global variable table
 * - Constant table
 * - Function import table
 * - Function export table (done)
 * - Variable import table
 * - Variable export table
 */

typedef struct Instruction {
    char* instr;                // Instruction string
    char* arg0, * arg1, * arg2; // Optional arguments for instruction
    bool is_label;              // Ugly hack to allow labels in the stream, instr will be label name
    struct Instruction* next;   // We won't be changing any instructions once they're generated
                                // so with a linked list we avoid size checks
} Instruction;

typedef struct FunExport {
    char* name;                 // Name of exported function
    char* type;                 // Return type of function
    char** args;                // Argument types
    bool is_main;               // From example, it seems "main" should be appended if function is main?
    struct FunExport* next;
} FunExport;

typedef struct {
    Instruction* instrs;
    Instruction* last_instr;
    FunExport* fun_exports;
    FunExport* last_fun_export;
} Assembly;

void ASMinit(Assembly* assembly);
void ASMfree(Assembly** assembly_ptr);
void ASMemitInstr(Assembly* assembly, char* instr_name, char* arg0, char* arg1, char* arg2, bool is_label);
