/**
 * @file
 *
 * Traversal: ByteCodeGeneration
 * UID      : BC
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

#include "common.h"
#include "asm.h"
#include "writer.h"
#include "global/globals.h"
#include "symbol/scopetree.h"
#include "symbol/table.h"

typedef enum Origin {
    GLOBAL_ORIGIN,
    LOCAL_ORIGIN,
    IMPORTED_ORIGIN,
} Origin;

static FILE* ASM_FILE;
static Assembly ASM;

static SymbolTable* CURRENT_SCOPE;

static size_t CONST_COUNT = 0;

static size_t NUMBERED_LABEL_COUNT = 0;

static ValueType LAST_TYPE = VT_NULL;
static bool HAD_EXPR = false;

// Checks whether a return statement is issued (or if we need to implicitly add one in case of void)
static bool HAD_RETURN = false;

/**
 * Emits instruction; shortcut to prevent manually passing ASM pointer
 * @param instr_name name of instruction
 * @param arg0 optional first argument
 * @param arg1 optional second argument
 * @param arg2 optional third argument
 */
void Instr(char* instr_name, char* arg0, char* arg1, char* arg2) {
    if (CURRENT_SCOPE->nesting_level == 0) {
        ASMemitInit(&ASM, instr_name, arg0, arg1, arg2);
    } else {
        ASMemitInstr(&ASM, instr_name, arg0, arg1, arg2);
    }
}

/**
 * Emits label; shortcut to prevent manually passing ASM pointer
 * @param label label name
 * @param is_fun boolean indicating whether label is a function start of regular label
 */
void Label(char* label, const bool is_fun) {
    ASMemitLabel(&ASM, label, is_fun);
}

/**
 * Creates an array of valuetypes represented as strings
 * @param vts pointer to array of valuetypes
 * @param len length of array
 * @return pointer to array of strings of valuetypes
 */
char** generate_vt_strs(const ValueType* vts, const size_t len) {
    char** strs = MEMmalloc(len * sizeof(char*));
    for (size_t i = 0; i < len; i++) {
        strs[i] = vt_to_str(vts[i]);
    }
    return strs;
}

/**
 * Finds a known function import in the import table and returns its entry in the table
 * @param name name of function import to find
 * @return found entry in function import table
 */
static FunImportEntry find_fun_import(const char* name) {
    const FunImportEntry res = ASMfindFunImport(&ASM, name);
#ifdef DEBUGGING
    ASSERT_MSG((res.get != 0), "Result of retrieving known imported function yielded no results");
#endif // DEBUGGING
    return res;
}

/**
 * Generates a unique label name that is guaranteed not to collide with
 * any existing names
 * @param name name to append to unique part of name
 * @return unique name
 */
char* generate_label_name(char* name) {
    char* res = safe_concat_str(STRcpy("_lab"), int_to_str((int) NUMBERED_LABEL_COUNT++));
    res = safe_concat_str(res, STRcpy("_"));
    return safe_concat_str(res, name);
}

/**
 * Loads the correct array reference to the stack
 * @param arr array to load reference
 */
static void load_array_ref(const Symbol* arr) {
    char* offset_str = int_to_str((int) arr->offset);
    if (arr->imported) {
        Instr("aloade", offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == 0) {
        Instr("aloadg", offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        Instr("aload", offset_str, NULL, NULL);
    } else {
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - arr->parent_scope->nesting_level));
        Instr("aloadn", nesting_diff_str, offset_str, NULL);
        MEMfree(nesting_diff_str);
    }
    MEMfree(offset_str);
}

/**
 * Emits the correct instruction for storing a value into an array reference
 * @param arr array symbol
 */
static void store_array_ref_with_value(const Symbol* arr) {
    switch (arr->vtype) {
        case VT_NUMARRAY: Instr("istorea", NULL, NULL, NULL); break;
        case VT_FLOATARRAY: Instr("fstorea", NULL, NULL, NULL); break;
        case VT_BOOLARRAY: Instr("bstorea", NULL, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Unexpected array vtype %s", vt_to_str(arr->vtype));
#endif // DEBUGGING
    }
}

/**
 * Emits the correct instruction for loading a value from an array reference
 * @param arr array symbol
 */
static void load_array_ref_with_value(const Symbol* arr) {
    switch (arr->vtype) {
        case VT_NUMARRAY: Instr("iloada", NULL, NULL, NULL); break;
        case VT_FLOATARRAY: Instr("floada", NULL, NULL, NULL); break;
        case VT_BOOLARRAY: Instr("bloada", NULL, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Unexpected array vtype %s", vt_to_str(arr->vtype));
#endif // DEBUGGING
    }
}

/**
 * Pushes single dimension value onto the stack
 * @param dim dim symbol to push
 */
static void push_array_dim(const Symbol* dim) {
    char* instr;
    char* offset_str = int_to_str((int) dim->offset);
    if (dim->imported) {
        instr = "iloade";
    } else if (dim->parent_scope->nesting_level == 0) {
        instr = "iloadg";
    } else if (dim->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        instr = "iload";
    } else {
        instr = "iloadn";
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - dim->parent_scope->nesting_level));
        Instr(instr, nesting_diff_str, offset_str, NULL);

        MEMfree(nesting_diff_str);
        MEMfree(offset_str);
        // Skip rest of the function due to differing instruction format
        return;
    }

    Instr(instr, offset_str, NULL, NULL);
    MEMfree(offset_str);
}

/**
 * Pushes all array dimensions starting from starting index onto the stack
 * @param arr array symbol
 * @param start start index
 */
static void push_array_dims(const Symbol* arr, const size_t start) {
    for (size_t i = start; i < arr->as.array.dim_count; i++) {
        const Symbol* dim = arr->as.array.dims[i];

        push_array_dim(dim);
    }
}

/**
 * Emits the correct instructions for computing the size of an array at runtime
 * @param arr array symbol
 */
static void comp_array_size(const Symbol* arr) {
    push_array_dims(arr, 0);
    for (size_t i = 1; i < arr->as.array.dim_count; i++) Instr("imul", NULL, NULL, NULL);
}

/**
 * Stores single dimension value from stack into variable
 * @param dim dim symbol to store into
 */
static void store_array_dim(const Symbol* dim) {
    char* instr;
    char* offset_str = int_to_str((int) dim->offset);
    if (dim->imported) {
        instr = "istoree";
    } else if (dim->parent_scope->nesting_level == 0) {
        instr = "istoreg";
    } else if (dim->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        instr = "istore";
    } else {
        instr = "istoren";
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - dim->parent_scope->nesting_level));
        Instr(instr, nesting_diff_str, offset_str, NULL);

        MEMfree(nesting_diff_str);
        MEMfree(offset_str);
        // Skip rest of the loop due to differing instruction format
        return;
    }

    Instr(instr, offset_str, NULL, NULL);
    MEMfree(offset_str);
}

/**
 * Pushes all array dimensions onto the stack and then the array reference itself
 * @param arr array whose dimensions to push onto stack
 */
static void push_array_with_dims(const Symbol* arr) {
    push_array_dims(arr, 0);

    char* instr;
    char* offset_str = int_to_str((int) arr->offset);
    if (arr->imported) {
        instr = "aloade";
    } else if (arr->parent_scope->nesting_level == 0) {
        instr = "aloadg";
    } else if (arr->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        instr = "aload";
    } else {
        instr = "aloadn";
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - arr->parent_scope->nesting_level));
        Instr(instr, nesting_diff_str, offset_str, NULL);

        MEMfree(nesting_diff_str);
        MEMfree(offset_str);
        // Return due to differing instruction format
        return;
    }

    Instr(instr, offset_str, NULL, NULL);
    MEMfree(offset_str);
}

/**
 * Fills all array size parameters with the correct values
 * @param arr array to populate
 * @param exprs_node first Exprs node of the arrays dimensions
 */
static void fill_array_dims(const Symbol* arr, node_st* exprs_node) {
    for (size_t i = 0; i < arr->as.array.dim_count; i++) {
        const Symbol* dim = arr->as.array.dims[i];

        // Push value onto stack, should evaluate to integer
        TRAVexpr(exprs_node);
#ifdef DEBUGGING
        ASSERT_MSG((LAST_TYPE == VT_NUM), "Array dimension did not evaluate to int");
#endif // DEBUGGING

        // Save value
        store_array_dim(dim);

        exprs_node = EXPRS_NEXT(exprs_node);
    }
}

/**
 * Flattens exprs into flat array index for multidimensional arrays
 * @param arr array symbol
 * @param exprs_node starting exprs node
 */
static void flatten_dim_exprs(const Symbol* arr, node_st* exprs_node) {
    for (size_t i = 0; i < arr->as.array.dim_count; i++) {
        // Find index
        TRAVexpr(exprs_node);

        // Multiply index with mul-result of all next dim sizes for flattening
        if (i < arr->as.array.dim_count - 1) {
            push_array_dims(arr, i + 1);
            for (size_t j = i + 1; j < arr->as.array.dim_count; j++) Instr("imul", NULL, NULL, NULL);
        }

        // Add together with prev size
        if (i != 0) Instr("iadd", NULL, NULL, NULL);

        exprs_node = EXPRS_NEXT(exprs_node);
    }
}

/**
 * Instantiates an array in bytecode. Pushes its dimensions,
 * which must already have been saved, and then multiplies
 * them so the total size is reached. Then creates array with
 * size and stores array to array offset
 * @param arr array symbol
 */
static void create_array_with_size(const Symbol* arr) {
    // Push all values
    push_array_dims(arr, 0);

    // Push n_values - 1 imul instructions
    for (size_t i = 1; i < arr->as.array.dim_count; i++) Instr("imul", NULL, NULL, NULL);

    // Store array size
    switch (arr->vtype) {
        case VT_NUMARRAY: Instr("inewa", NULL, NULL, NULL); break;
        case VT_FLOATARRAY: Instr("fnewa", NULL, NULL, NULL); break;
        case VT_BOOLARRAY: Instr("bnewa", NULL, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Unexpected array type %s", vt_to_str(arr->vtype));
#endif // DEBUGGING
    }

    // Store array reference
    char* offset_str = int_to_str((int) arr->offset);
    if (arr->imported) {
        Instr("astoree", offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == 0) {
        Instr("astoreg", offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        Instr("astore", offset_str, NULL, NULL);
    } else {
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - arr->parent_scope->nesting_level));
        Instr("astoren", nesting_diff_str, offset_str, NULL);
        MEMfree(nesting_diff_str);
    }
    MEMfree(offset_str);
}

/**
 * Initialises an array with a single scalar
 * @param arr array symbol
 */
static void init_array_with_scalar(const Symbol* arr) {
    // Expr must have been traversed and on stack top

    // Scalar value variable
    char* scalar_symbol_name = safe_concat_str(STRcpy("_scalar_"), STRcpy(arr->name));
    const Symbol* scalar_symbol = STlookup(arr->parent_scope, scalar_symbol_name);
    MEMfree(scalar_symbol_name);
    char* scalar_offset_str = int_to_str((int) scalar_symbol->offset);

    // Loop counter variable
    char* counter_symbol_name = safe_concat_str(STRcpy("_counter_"), STRcpy(arr->name));
    const Symbol* counter_symbol = STlookup(arr->parent_scope, counter_symbol_name);
    MEMfree(counter_symbol_name);
    char* counter_offset_str = int_to_str((int) counter_symbol->offset);

    // Array size variable
    char* size_symbol_name = safe_concat_str(STRcpy("_size_"), STRcpy(arr->name));
    const Symbol* size_symbol = STlookup(arr->parent_scope, size_symbol_name);
    MEMfree(size_symbol_name);
    char* size_offset_str = int_to_str((int) size_symbol->offset);

    // Array variable
    char* arr_offset_str = int_to_str((int) arr->offset);

    char* for_loop_start_name = generate_label_name(STRcpy("for_loop_start"));
    char* for_loop_end_name = generate_label_name(STRcpy("for_loop_end"));

    // Save expr to expr variable
    switch (LAST_TYPE) {
        case VT_NUM: Instr("istore", scalar_offset_str, NULL, NULL); break;
        case VT_FLOAT: Instr("fstore", scalar_offset_str, NULL, NULL); break;
        case VT_BOOL: Instr("bstore", scalar_offset_str, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Trying to scalar-init a variable with a non-scalar");
#endif
    }

    // Save loop counter (zero)
    Instr("iloadc_0", NULL, NULL, NULL);
    Instr("istore", counter_offset_str, NULL, NULL);

    // Save array size (end value for counter)
    comp_array_size(arr);
    Instr("istore", size_offset_str, NULL, NULL);

    // --- START FOR LOOP
    // Emit label
    Label(for_loop_start_name, false);

    // Check loop condition
    Instr("iload", counter_offset_str, NULL, NULL);
    Instr("iload", size_offset_str, NULL, NULL);
    Instr("ilt", NULL, NULL, NULL);
    Instr("branch_f", for_loop_end_name, NULL, NULL);

    // Save scalar to array at index [counter]
    // Load scalar
    switch (LAST_TYPE) {
        case VT_NUM: Instr("iload", scalar_offset_str, NULL, NULL); break;
        case VT_FLOAT: Instr("fload", scalar_offset_str, NULL, NULL); break;
        case VT_BOOL: Instr("bload", scalar_offset_str, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Trying to scalar-init a variable with a non-scalar");
#endif
    }

    // Load array index (counter) + load array reference
    if (arr->imported) {
        Instr("iloade", counter_offset_str, NULL, NULL);
        Instr("aloade", arr_offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == 0) {
        Instr("iloadg", counter_offset_str, NULL, NULL);
        Instr("aloadg", arr_offset_str, NULL, NULL);
    } else if (arr->parent_scope->nesting_level == CURRENT_SCOPE->nesting_level) {
        Instr("iload", counter_offset_str, NULL, NULL);
        Instr("aload", arr_offset_str, NULL, NULL);
    } else {
        char* nesting_diff_str = int_to_str((int) (CURRENT_SCOPE->nesting_level - arr->parent_scope->nesting_level));
        Instr("iloadn", nesting_diff_str, counter_offset_str, NULL);
        Instr("aloadn", nesting_diff_str, arr_offset_str, NULL);
    }

    // Store value
    switch (LAST_TYPE) {
        case VT_NUM: Instr("istorea", NULL, NULL, NULL); break;
        case VT_FLOAT: Instr("fstorea", NULL, NULL, NULL); break;
        case VT_BOOL: Instr("bstorea", NULL, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Trying to scalar-init a variable with a non-scalar");
#endif
    }

    // Increment counter
    Instr("iinc_1", counter_offset_str, NULL, NULL);

    // Jump back
    Instr("jump", for_loop_start_name, NULL, NULL);

    // --- END FOR LOOP
    // Emit label
    Label(for_loop_end_name, false);

    MEMfree(scalar_offset_str);
    MEMfree(counter_offset_str);
    MEMfree(size_offset_str);
    MEMfree(arr_offset_str);
    MEMfree(for_loop_start_name);
    MEMfree(for_loop_end_name);
}

/**
 * Counts the amount of expressions an arrexpr contains. Also explores nested
 * arrexprs and errors on inconsistent size.
 * @param node arrexpr node used to initialise array
 * @return amount of expressions (recursively) contained by the arrexpr
 */
static size_t count_arrexpr(node_st* node) {
    size_t count = 0;

    const enum ccn_nodetype type = NODE_TYPE(node);

    if (type == NT_ARREXPR) {
        return count_arrexpr(ARREXPR_EXPRS(node));
    }

    if (type == NT_EXPRS) {
        bool only_arrexpr = false;
        while (node != NULL) {
            if (NODE_TYPE(EXPRS_EXPR(node)) == NT_ARREXPR) {
                count += count_arrexpr(EXPRS_EXPR(node));
                only_arrexpr = true;
            } else {
                count++;
                if (only_arrexpr) {
                    USER_ERROR("Inconsistent initialisation value shape of array");
                    exit(EXIT_FAILURE);
                }
            }

            node = EXPRS_NEXT(node);
        }
    }

    return count;
}

/**
 * Initialises an array with an arrexpr. Supports nested arrexprs.
 * @param arr array symbol
 * @param count amout of initialisation exprs expected
 */
static void init_array_with_arrexpr(const Symbol* arr, const size_t count) {
    // Arrexprs must have been traversed and all stored on stack
    // We save them in reverse starting from the last initialised array index

    char* instr;
    switch (arr->vtype) {
        case VT_NUMARRAY: instr = "istorea"; break;
        case VT_FLOATARRAY: instr = "fstorea"; break;
        case VT_BOOLARRAY: instr = "bstorea"; break;
        default:
#ifdef DEBUGGING
            ERROR("Unexpected array vtype %s", vt_to_str(arr->vtype));
#endif
    }

    char* arr_offset_str = int_to_str((int) arr->offset);

    for (int idx = (int) count - 1; idx >= 0; idx--) {
        // Push index
        char* idx_str = int_to_str(idx);
        ASMemitConst(&ASM, "int", idx_str);

        const size_t idx_offset = CONST_COUNT++;
        char* idx_offset_str = int_to_str((int) idx_offset);

        Instr("iloadc", idx_offset_str, NULL, NULL);

        // Push array reference
        load_array_ref(arr);

        // Save value
        Instr(instr, NULL, NULL, NULL);

        MEMfree(idx_str);
        MEMfree(idx_offset_str);
    }

    MEMfree(arr_offset_str);
}

static void init() {
    CURRENT_SCOPE = GB_GLOBAL_SCOPE;
}

static void fini() {
    // Write assembly output
    ASM_FILE = fopen(global.output_file, "w");
    if (ASM_FILE == NULL) {
        fprintf(stderr, "Error creating bytecode file");
        exit(1);
    }
    write_assembly(ASM_FILE, &ASM);
    fclose(ASM_FILE);

    // Free memory
    STfree(&GB_GLOBAL_SCOPE);
}

/**
 * @fn BCprogram
 */
node_st *BCprogram(node_st *node)
{
    init();

    if (GB_REQUIRES_INIT_FUNCTION) {
        ASMemitFunExport(&ASM, "__init", "void", 0, NULL);
    }

    TRAVchildren(node);

    // Write collected ASM to file
    fini();

    return node;
}

/**
 * @fn BCdecls
 */
node_st *BCdecls(node_st *node)
{
    TRAVchildren(node);

    /**
     * Do nothing
     */
    return node;
}

/**
 * @fn BCexprs
 */
node_st *BCexprs(node_st *node)
{
    TRAVchildren(node);

    /**
     * Do nothing
     */
    return node;
}

/**
 * @fn BCarrexpr
 */
node_st *BCarrexpr(node_st *node)
{
    TRAVchildren(node);

    /**
     * Do nothing
     */
    return node;
}

/**
 * @fn BCids
 */
node_st *BCids(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCexprstmt
 */
node_st *BCexprstmt(node_st *node)
{
    TRAVchildren(node);

    switch (LAST_TYPE) {
        case VT_NUM: Instr("ipop", NULL, NULL, NULL); break;
        case VT_FLOAT: Instr("fpop", NULL, NULL, NULL); break;
        case VT_BOOL: Instr("bpop", NULL, NULL, NULL); break;
        case VT_VOID: break;  // Special case: void function does not return anything to needs be popped
        default:  // Should never occur
#ifdef DEBUGGING
            ERROR("Unexpected exprstmt type %s", vt_to_str(LAST_TYPE));
#endif // DEBUGGING
    }

    /**
     * Pop value, it wasn't assigned to anything
     */
    return node;
}

/**
 * @fn BCreturn
 */
node_st *BCreturn(node_st *node)
{
    TRAVchildren(node);

    switch (CURRENT_SCOPE->parent_fun->vtype) {
        case VT_NUM: Instr("ireturn", NULL, NULL, NULL); break;
        case VT_FLOAT: Instr("freturn", NULL, NULL, NULL); break;
        case VT_BOOL: Instr("breturn", NULL, NULL, NULL); break;
        case VT_VOID: Instr("return", NULL, NULL, NULL); break;
        default:  // Should never occur
#ifdef DEBUGGING
            ERROR("Unexpected return valuetype %i", CURRENT_SCOPE->parent_fun->vtype);
#endif // DEBUGGING
    }

    HAD_RETURN = true;

    /**
     * Emit return instruction (with correct type)
     */
    return node;
}

/**
 * @fn BCfuncall
 */
node_st *BCfuncall(node_st *node)
{
    char* name = FUNCALL_NAME(node);
    const Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);
#ifdef DEBUGGING
    ASSERT_MSG((s != NULL), "BYTECODE: Could not find symbol named %s", name);
#endif // DEBUGGING

    const size_t current_level = CURRENT_SCOPE->parent_fun->parent_scope->nesting_level;
    const size_t fun_level = s->parent_scope->nesting_level;

    if (fun_level == 0) {
        // Global function
        Instr("isrg", NULL, NULL, NULL);
    } else if (fun_level == current_level + 1) {
        // Function defined inside current scope
        Instr("isrl", NULL, NULL, NULL);
    } else if (current_level == fun_level) {
        // "Sister function", both defined in the same scope
        Instr("isr", NULL, NULL, NULL);
    } else {
#ifdef DEBUGGING
        ASSERT_MSG((current_level >= fun_level + 1), "Calling function from scope depth %lu unreachable by own scope depth %lu",
            fun_level, current_level);
#endif // DEBUGGING
        char* delta_level = int_to_str((int) (current_level - fun_level));
        Instr("isrn", delta_level, NULL, NULL);
        MEMfree(delta_level);
    }

    TRAVchildren(node);

    if (s->imported) {
        const size_t offset = find_fun_import(name).offset;
        char* offset_str = int_to_str((int) offset);
        Instr("jsre", offset_str, NULL, NULL);
        MEMfree(offset_str);
    } else {
        char* var_count_str = int_to_str((int) s->as.fun.param_count);
#ifdef DEBUGGING
        ASSERT_MSG((strcmp(s->as.fun.label_name, "\0") != 0), "Empty label name for fun %s", s->name);
#endif // DEBUGGING
        Instr("jsr", var_count_str, s->as.fun.label_name, NULL);
        MEMfree(var_count_str);
    }

    HAD_EXPR = true;
    LAST_TYPE = s->vtype;

    /**
     * For each argument:
     * -- Emit load for argument
     * Find out which scope function is from for funcall instruction?
     * Emit funcall instruction with amount of vars added
     */
    return node;
}

/**
 * @fn BCcast
 */
node_st *BCcast(node_st *node) {
    TRAVchildren(node);

    // INTEGER AND FLOAT
    if (LAST_TYPE == VT_NUM && CAST_TYPE(node) == CT_float) {
        Instr("i2f", NULL, NULL, NULL);
    } else if (LAST_TYPE == VT_FLOAT && CAST_TYPE(node) == CT_int) {
        Instr("f2i", NULL, NULL, NULL);
    }

    // BOOLEAN AND INTEGER
    else if (LAST_TYPE == VT_NUM && CAST_TYPE(node) == CT_bool) {
        /* Create instructions for
         * if ([lastvalue] != 0) {
         *     [push] true
         * } else {
         *     [push] false
         * }
         */

        char* else_label_name = generate_label_name(STRcpy("else"));
        char* endif_label_name = generate_label_name(STRcpy("end"));

        Instr("iloadc_0", NULL, NULL, NULL);
        Instr("ine", NULL, NULL, NULL);
        Instr("branch_f", else_label_name, NULL, NULL);
        Instr("bloadc_t", NULL, NULL, NULL);
        Instr("jump", endif_label_name, NULL, NULL);
        Label(else_label_name, false);
        Instr("bloadc_f", NULL, NULL, NULL);
        Label(endif_label_name, false);

        MEMfree(else_label_name);
        MEMfree(endif_label_name);
    } else if (LAST_TYPE == VT_BOOL && CAST_TYPE(node) == CT_int) {
        /* Create instructions for
         * if ([lastvalue]) {
         *     [push] 1
         * } else {
         *     [push] 0
         * }
         */

        char* else_label_name = generate_label_name(STRcpy("else"));
        char* endif_label_name = generate_label_name(STRcpy("end"));

        Instr("branch_f", else_label_name, NULL, NULL);
        Instr("iloadc_1", NULL, NULL, NULL);
        Instr("jump", endif_label_name, NULL, NULL);
        Label(else_label_name, false);
        Instr("iloadc_0", NULL, NULL, NULL);
        Label(endif_label_name, false);

        MEMfree(else_label_name);
        MEMfree(endif_label_name);
    }

    // BOOLEAN AND FLOAT
    else if (LAST_TYPE == VT_FLOAT && CAST_TYPE(node) == CT_bool) {
        /* Create instructions for
         * if ([lastvalue] != 0.0) {
         *     [push] true
         * } else {
         *     [push] false
         * }
         */

        char* else_label_name = generate_label_name(STRcpy("else"));
        char* endif_label_name = generate_label_name(STRcpy("end"));

        Instr("floadc_0", NULL, NULL, NULL);
        Instr("fne", NULL, NULL, NULL);
        Instr("branch_f", else_label_name, NULL, NULL);
        Instr("bloadc_t", NULL, NULL, NULL);
        Instr("jump", endif_label_name, NULL, NULL);
        Label(else_label_name, false);
        Instr("bloadc_f", NULL, NULL, NULL);
        Label(endif_label_name, false);

        MEMfree(else_label_name);
        MEMfree(endif_label_name);
    } else if (LAST_TYPE == VT_BOOL && CAST_TYPE(node) == CT_float) {
        /* Create instructions for
         * if ([lastvalue]) {
         *     [push] 1.0
         * } else {
         *     [push] 0.0
         * }
         */

        char* else_label_name = generate_label_name(STRcpy("else"));
        char* endif_label_name = generate_label_name(STRcpy("end"));

        Instr("branch_f", else_label_name, NULL, NULL);
        Instr("floadc_1", NULL, NULL, NULL);
        Instr("jump", endif_label_name, NULL, NULL);
        Label(else_label_name, false);
        Instr("floadc_0", NULL, NULL, NULL);
        Label(endif_label_name, false);

        MEMfree(else_label_name);
        MEMfree(endif_label_name);
    } else {
        // Should never occur
#ifdef DEBUGGING
        ERROR("Unexpected cast from VT type %s to CT type %i", vt_to_str(LAST_TYPE), CAST_TYPE(node));
#endif // DEBUGGING
    }

    LAST_TYPE = ct_to_vt(CAST_TYPE(node), false);

    /**
     * Traverse children
     * Emit cast
     */
    return node;
}

/**
 * @fn BCfundefs
 */
node_st *BCfundefs(node_st *node)
{
    TRAVchildren(node);

    /**
     * Do nothing
     */
    return node;
}

/**
 * @fn BCfundef
 */
node_st *BCfundef(node_st *node)
{
    char* name = FUNDEF_NAME(node);
    const Symbol* fun_symbol = ScopeTreeFind(CURRENT_SCOPE, name);
    const FunData* fun_data = &fun_symbol->as.fun;

    // Save function to export list if export
    if (fun_symbol->exported) {
        ASMemitFunExport(
            &ASM,
            name,
            vt_to_str(fun_symbol->vtype),
            fun_data->param_count,
            generate_vt_strs(fun_data->param_types, fun_data->param_count));
    }

    // Save function to import list if import
    else if (fun_symbol->imported) {
        ASMemitFunImport(
            &ASM,
            name,
            vt_to_str(fun_symbol->vtype),
            fun_data->param_count,
            generate_vt_strs(fun_data->param_types, fun_data->param_count));
    }

    // Switch scopes and traverse if not external function
    if (!fun_symbol->imported) {
        // Switch scope
        SymbolTable* prev_scope = CURRENT_SCOPE;
        CURRENT_SCOPE = fun_symbol->as.fun.scope;

        TRAVchildren(node);

        // Reset loop counter
        CURRENT_SCOPE->for_loop_counter = 0;

        // Revert scope
        CURRENT_SCOPE = prev_scope;
    }


    /**
    * Switch scopes if not imported
    */
    return node;
}

/**
 * @fn BCfunbody
 */
node_st *BCfunbody(node_st *node)
{
    TRAVlocal_fundefs(node);

    HAD_RETURN = false;

    char* label_name = CURRENT_SCOPE->parent_fun->as.fun.label_name;
    Label(label_name, true);

    // Only write "esr" if at least one variable (NOT PARAMETER) will be initialised
    if (CURRENT_SCOPE->localvar_offset_counter
        - CURRENT_SCOPE->parent_fun->as.fun.param_count > 0) {

        char* offset_str = int_to_str((int) CURRENT_SCOPE->localvar_offset_counter);
        Instr("esr", offset_str, NULL, NULL);
        MEMfree(offset_str);
    }

    TRAVdecls(node);
    TRAVstmts(node);

    // Add a void return if the code doesn't contain a return statement
    if (CURRENT_SCOPE->parent_fun->vtype == VT_VOID) {
        if (!HAD_RETURN) {
            Instr("return", NULL, NULL, NULL);
        }
    }

    /**
     * Traverse nested functions (otherwise functions mix in bytecode)
     * Emit function label
     * Emit "esr N" with N being the amount of variables we are going to use in function
     * Traverse decls and stmts
     */
    return node;
}

/**
 * @fn BCifelse
 */
node_st *BCifelse(node_st *node)
{
    char* else_label_name = generate_label_name(STRcpy("else"));
    char* endif_label_name = generate_label_name(STRcpy("end"));

    TRAVcond(node);

    Instr("branch_f", else_label_name, NULL, NULL);

    TRAVthen(node);

    Instr("jump", endif_label_name, NULL, NULL);
    Label(else_label_name, false);

    TRAVelse_block(node);

    Label(endif_label_name, false);

    MEMfree(else_label_name);
    MEMfree(endif_label_name);

    /**
     * Traverse cond child
     * Emit jump to label else if false
     * Traverse then child
     * Emit unconditional jump to label endif
     * Emit label else
     * Traverse else child (might be empty which yields same result following these steps)
     * Emit label endif
     */
    return node;
}

/**
 * @fn BCwhile
 */
node_st *BCwhile(node_st *node)
{
    char* while_start_name = generate_label_name(STRcpy("while_loop_start"));
    char* while_end_name = generate_label_name(STRcpy("while_loop_end"));

    Label(while_start_name, false);

    TRAVcond(node);

    Instr("branch_f", while_end_name, NULL, NULL);

    TRAVblock(node);

    Instr("jump", while_start_name, NULL, NULL);

    Label(while_end_name, false);

    MEMfree(while_start_name);
    MEMfree(while_end_name);

    /**
     * Emit loop start label
     * Traverse cond child
     * Emit jump to loop end label if false (expr will have resolved to boolean)
     * Traverse body
     * Emit unconditional jump to loop start label
     * Emit loop end label
     */
    return node;
}

/**
 * @fn BCdowhile
 */
node_st *BCdowhile(node_st *node)
{
    char* while_start_name = generate_label_name(STRcpy("while_loop_start"));

    Label(while_start_name, false);

    TRAVblock(node);

    TRAVcond(node);

    Instr("branch_t", while_start_name, NULL, NULL);

    MEMfree(while_start_name);

    /**
     * Emit loop start label
     * Traverse body
     * Traverse cond child
     * Emit jump to loop end label if false (expr will have resolved to boolean)
     * Emit unconditional jump to loop start label
     * Emit loop end label
     */

    return node;
}

/**
 * @fn BCfor
 */
node_st *BCfor(node_st *node)
{
    // Switch to loop scope
    char* name = FOR_VAR(node);
    char* adjusted_name = safe_concat_str(
        int_to_str((int) CURRENT_SCOPE->for_loop_counter),
        safe_concat_str(STRcpy("_"), STRcpy(name)));
    const Symbol* s_loop = STlookup(CURRENT_SCOPE, adjusted_name);
    CURRENT_SCOPE = s_loop->as.forloop.scope;

    // Place correct values in all variables
    TRAVstart_expr(node);
#ifdef DEBUGGING
    ASSERT_MSG((LAST_TYPE == VT_NUM), "Got a non-integer value for loop start expression");
#endif // DEBUGGING
    const Symbol* s_counter = STlookup(CURRENT_SCOPE, name);
    char* loop_offset_str = int_to_str((int) s_counter->offset);
    Instr("istore", loop_offset_str, NULL, NULL);

    TRAVstop(node);
#ifdef DEBUGGING
    ASSERT_MSG((LAST_TYPE == VT_NUM), "Got a non-integer value for loop stop condition");
#endif // DEBUGGING
    const Symbol* s_cond = STlookup(CURRENT_SCOPE, "_cond");
    char* cond_offset_str = int_to_str((int) s_cond->offset);
    Instr("istore", cond_offset_str, NULL, NULL);

    TRAVstep(node);
#ifdef DEBUGGING
    ASSERT_MSG((LAST_TYPE == VT_NUM), "Got a non-integer value for loop step expression");
#endif // DEBUGGING
    const Symbol* s_step = STlookup(CURRENT_SCOPE, "_step");
    char* step_offset_str = int_to_str((int) s_step->offset);
    Instr("istore", step_offset_str, NULL, NULL);

    // Generate bytecode
    char* for_loop_start_name = generate_label_name(STRcpy("for_loop_start"));
    char* positive_step_size_cond = generate_label_name(STRcpy("positive_step_size"));
    char* negative_step_size_cond = generate_label_name(STRcpy("negative_step_size"));
    char* for_loop_common_cond_check = generate_label_name(STRcpy("common_cond_check"));
    char* for_loop_end_name = generate_label_name(STRcpy("for_loop_end"));

    // Emit loop start label
    Label(for_loop_start_name, false);

    // Evaluate loop condition
    // --- Perform sign check
    Instr("iload", step_offset_str, NULL, NULL);
    Instr("iloadc_0", NULL, NULL, NULL);
    Instr("ige", NULL, NULL, NULL);
    Instr("branch_t", positive_step_size_cond, NULL, NULL);
    Instr("jump", negative_step_size_cond, NULL, NULL);

    // --- Check for positive step size case
    Label(positive_step_size_cond, false);
    Instr("iload", loop_offset_str, NULL, NULL);
    Instr("iload", cond_offset_str, NULL, NULL);
    Instr("ilt", NULL, NULL, NULL);
    Instr("jump", for_loop_common_cond_check, NULL, NULL);

    // Check for negative step size case
    Label(negative_step_size_cond, false);
    Instr("iload", loop_offset_str, NULL, NULL);
    Instr("iload", cond_offset_str, NULL, NULL);
    Instr("igt", NULL, NULL, NULL);

    // Common check, loop exits here if false
    Label(for_loop_common_cond_check, false);
    Instr("branch_f", for_loop_end_name, NULL, NULL);

    // Evaluate body
    TRAVblock(node);

    // Increment value of loop variable
    TRAVstep(node);
    Instr("iload", loop_offset_str, NULL, NULL);
    Instr("iadd", NULL, NULL, NULL);
    Instr("istore", loop_offset_str, NULL, NULL);

    // Unconditional jump back to loop start (expression evaluation)
    Instr("jump", for_loop_start_name, NULL, NULL);

    // Emit loop end label
    Label(for_loop_end_name, false);

    // Special case: restore loop counter to zero for next traversal
    CURRENT_SCOPE->for_loop_counter = 0;

    // Restore scope
    CURRENT_SCOPE = CURRENT_SCOPE->parent_scope;

    // Increment loop counter for next for-loop
    CURRENT_SCOPE->for_loop_counter++;

    // Clean up
    MEMfree(adjusted_name);
    MEMfree(for_loop_start_name);
    MEMfree(positive_step_size_cond);
    MEMfree(negative_step_size_cond);
    MEMfree(for_loop_common_cond_check);
    MEMfree(for_loop_end_name);

    MEMfree(loop_offset_str);
    MEMfree(cond_offset_str);
    MEMfree(step_offset_str);

    /**
     * Traverse init, cond and step children
     * Store init value (already emitted by child) in loop var
     * Store cond value (already emitted by child) in stack
     * Store step value (already emitted by child) in stack
     * Emit while label
     * Emit load instruction for loop var
     * Emit load instruction for cond value
     * Emit "if less than"
     * Emit jump to branch end (will trigger if loop should be broken)
     * Traverse body
     * Emit instruction for loop var increment
     * Emit jump to while label
     * Emit end while label
     */
    return node;
}

/**
 * @fn BCglobdecl
 */
node_st *BCglobdecl(node_st *node)
{
    // Add to import list
    char* name = GLOBDECL_NAME(node);
    const Symbol* var_symbol = ScopeTreeFind(CURRENT_SCOPE, name);

    // Add dims before array
    if (var_symbol->stype == ST_ARRAYVAR) {
        char* num_str = vt_to_str(VT_NUM);
        for (size_t i = 0; i < var_symbol->as.array.dim_count; i++) {
            char* id_name = generate_array_dim_name(name, i);
            ASMemitVarImport(&ASM, id_name, num_str);
            MEMfree(id_name);
        }
    }

    ASMemitVarImport(
        &ASM,
        name,
        vt_to_str(var_symbol->vtype));

    /**
     * Save to imported variables stack
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCglobdef
 */
node_st *BCglobdef(node_st *node)
{
    char* name = GLOBDEF_NAME(node);
    const Symbol* s = STlookup(CURRENT_SCOPE, name);
#ifdef DEBUGGING
    ASSERT_MSG((s != NULL), "Bytecode: Couldn't find globdef symbol in scope");
#endif // DEBUGGING

    if (s->stype == ST_ARRAYVAR) {
        for (size_t i = 0; i < s->as.array.dim_count; i++) {
            ASMemitGlobVar(&ASM, vt_to_str(VT_NUM));
        }
    }

    ASMemitGlobVar(
        &ASM,
        vt_to_str(s->vtype));

    if (GLOBDEF_EXPORT(node)) {
        if (s->stype == ST_ARRAYVAR) {
            for (size_t i = 0; i < s->as.array.dim_count; i++) {
                const Symbol* s_dim = s->as.array.dims[i];
                ASMemitVarExport(&ASM, s_dim->name, s_dim->offset);
            }
        }

        ASMemitVarExport(
            &ASM,
            s->name,
            s->offset);
    }

    // In case of array, store dimensions
    if (s->stype == ST_ARRAYVAR) {
        fill_array_dims(s, GLOBDEF_DIMS(node));
        create_array_with_size(s);
    }

    // Early return if no init
    if (GLOBDEF_INIT(node) == NULL) {
        return node;
    }

    TRAVinit(node);

    if (s->stype == ST_ARRAYVAR) {
        // Separate handling for arrays

        if (GLOBDEF_INIT(node) != NULL) {
            if (NODE_TYPE(GLOBDEF_INIT(node)) == NT_ARREXPR) {
                // Arrexpr
                init_array_with_arrexpr(s, count_arrexpr(GLOBDEF_INIT(node)));
            } else {
                // Scalar
                init_array_with_scalar(s);
            }

            return node;
        }
    }

    char* offset_str = int_to_str((int) s->offset);
    switch (LAST_TYPE) {
        case VT_NUM: Instr("istoreg", offset_str, NULL, NULL); break;
        case VT_FLOAT: Instr("fstoreg", offset_str, NULL, NULL); break;
        case VT_BOOL: Instr("bstoreg", offset_str, NULL, NULL); break;
        default:
#ifdef DEBUGGING
            ERROR("Bytecode: Unexpected globdef type %s", vt_to_str(LAST_TYPE));
#endif // DEBUGGING
    }

    MEMfree(offset_str);

    TRAVdims(node);

    /**
     * Match whether it has an expression:
     * --- No: Probably return since it's only here for the compiler to know the type
     * --- Yes: Treat as an assign; push value to globals stack
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCparam
 */
node_st *BCparam(node_st *node)
{
    TRAVchildren(node);



    /**
     * Variable as specified in funcall argument might already be on stack?
     * !!! Check this because I actually have no clue
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCvardecl
 */
node_st *BCvardecl(node_st *node)
{
    // Look up symbol and variabletype
    char* name = VARDECL_NAME(node);
    const Symbol* s = STlookup(CURRENT_SCOPE, name);
#ifdef DEBUGGING
    ASSERT_MSG((s != NULL), "BYTECODE: Could not find symbol named %s", name);
#endif // DEBUGGING

    // In case of array, store dimensions
    if (s->stype == ST_ARRAYVAR) {
        fill_array_dims(s, VARDECL_DIMS(node));
        create_array_with_size(s);
    }

    // Early return if no init
    if (VARDECL_INIT(node) == NULL) {
        TRAVnext(node);
        return node;
    }

    TRAVinit(node);

    if (s->stype == ST_ARRAYVAR) {
        // Separate handling for arrays

        if (VARDECL_INIT(node) != NULL) {
            if (NODE_TYPE(VARDECL_INIT(node)) == NT_ARREXPR) {
                // Arrexpr
                init_array_with_arrexpr(s, count_arrexpr(VARDECL_INIT(node)));
            } else {
                // Scalar
                init_array_with_scalar(s);
            }

            TRAVnext(node);
            return node;
        }
    }

    const size_t current_level = CURRENT_SCOPE->nesting_level;
    const size_t var_level = s->parent_scope->nesting_level;
    const size_t var_offset = s->offset;
#ifdef DEBUGGING
    ASSERT_MSG((current_level == var_level),
        "BYTECODE: Symbol declaration %s, only found in different scope",
        name);
#endif // DEBUGGING

    char* instr = NULL;
    switch (s->vtype) {
        case VT_NUM: instr = STRcpy("i"); LAST_TYPE = VT_NUM; break;
        case VT_FLOAT: instr = STRcpy("f"); LAST_TYPE = VT_FLOAT; break;
        case VT_BOOL: instr = STRcpy("b"); LAST_TYPE = VT_BOOL; break;
        default:  // Should not occur
#ifdef DEBUGGING
            ERROR("Incompatible Vardecl node with valuetype of %i", s->vtype);
#endif // DEBUGGING
    }

    instr = safe_concat_str(instr, STRcpy("store"));
    char* var_offset_str = int_to_str((int) var_offset);
    Instr(instr, var_offset_str, NULL, NULL);
    MEMfree(instr);
    MEMfree(var_offset_str);

    TRAVnext(node);

    /**
     * Match whether it has an expression:
     * --- No: Probably return since it's only here for the compiler to know the type
     * --- Yes: Treat as an assign; store value to variable
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCstmts
 */
node_st *BCstmts(node_st *node)
{
    TRAVchildren(node);

    /**
     * Do nothing
     */
    return node;
}

/**
 * @fn BCassign
 */
node_st *BCassign(node_st *node)
{
    TRAVexpr(node);
    TRAVlet(node);

    /**
     * Make sure expr is traversed before varlet
     */
    return node;
}

/**
 * @fn BCbinop
 */
node_st *BCbinop(node_st *node)
{
    // SEPARATE LOGIC FOR AND, AND OR SHORT-CIRCUITING
    const enum BinOpType t = BINOP_OP(node);
    if (t == BO_and) { // Short-circuit AND (&&)
        char* short_circuit_label = generate_label_name(STRcpy("else"));
        char* end_and_label = generate_label_name(STRcpy("end"));

        TRAVleft(node);
        const ValueType left_value = LAST_TYPE;
#ifdef DEBUGGING
        ASSERT_MSG((left_value == VT_BOOL), "Bytecode: Left operand of 'and' is not boolean");
#endif // DEBUGGING

        // If left operand is false, jump to short_circuit_label
        Instr("branch_f", short_circuit_label, NULL, NULL);

        // If left was true, evaluate right and perform AND (bmul)
        TRAVright(node);
        const ValueType right_value = LAST_TYPE;
#ifdef DEBUGGING
        ASSERT_MSG((right_value == VT_BOOL), "Bytecode: Right operand of 'and' is not boolean");
#endif // DEBUGGING

        // bmul instruction not necessary as next instruction will be the result (left was eliminated)
        Instr("jump", end_and_label, NULL, NULL); // Jump to the end

        Label(short_circuit_label, false);
        Instr("bloadc_f", NULL, NULL, NULL); // Load false directly for short-circuit

        Label(end_and_label, false);

        LAST_TYPE = VT_BOOL; // Result of AND is boolean

        MEMfree(short_circuit_label);
        MEMfree(end_and_label);

        return node;
    }

    if (t == BO_or) {
        // Short-circuit OR (||)
        char* short_circuit_label = generate_label_name(STRcpy("else"));
        char* end_or_label = generate_label_name(STRcpy("end"));

        TRAVleft(node);
        const ValueType left_value = LAST_TYPE;
#ifdef DEBUGGING
        ASSERT_MSG((left_value == VT_BOOL), "Left operand of 'or' is not boolean");
#endif // DEBUGGING
        // If left operand is true, jump to short_circuit_label
        Instr("branch_t", short_circuit_label, NULL, NULL);

        // If left was false, evaluate right and perform OR (badd)
        TRAVright(node);
        const ValueType right_value = LAST_TYPE;
#ifdef DEBUGGING
        ASSERT_MSG((right_value == VT_BOOL), "Right operand of 'or' is not boolean");
#endif // DEBUGGING

        // badd instruction not necessary as next instruction will be the result (left was eliminated)
        Instr("jump", end_or_label, NULL, NULL); // Jump to the end

        Label(short_circuit_label, false);
        Instr("bloadc_t", NULL, NULL, NULL); // Load true directly for short-circuit

        Label(end_or_label, false);

        LAST_TYPE = VT_BOOL; // Result of OR is boolean

        MEMfree(short_circuit_label);
        MEMfree(end_or_label);

        return node;
    }

    TRAVleft(node);
    const ValueType left_value = LAST_TYPE;
    TRAVright(node);
    const ValueType right_value = LAST_TYPE;

#ifdef DEBUGGING
    ASSERT_MSG((left_value == right_value), "Left value and right value of types %i and %i don't match",
        left_value, right_value);
#endif // DEBUGGING

    char* instr;
    switch (left_value) {
        case VT_NUM: instr = STRcpy("i"); break;
        case VT_FLOAT: instr = STRcpy("f"); break;
        case VT_BOOL: instr = STRcpy("b"); break;
        default:  // Should never occur
#ifdef DEBUGGING
            ERROR("Unexpected binop valuetype %i", left_value);
#endif
    }

    switch (t) {
        case BO_add:
            // badd is allowed; is logical disjunction of boolean values
            instr = safe_concat_str(instr, STRcpy("add"));
            break;
        case BO_sub:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("Subtraction was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("sub"));
            break;
        case BO_mul:
            // bmul is allowed; is logical conjunction of boolean values
            instr = safe_concat_str(instr, STRcpy("mul"));
            break;
        case BO_div:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("Division was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("div"));
            break;
        case BO_mod:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("Modulo was performed on boolean values");
            if (left_value == VT_FLOAT) ERROR("Modulo was performed on float values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("rem"));
            break;
        case BO_lt:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("< operator was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("lt"));
            LAST_TYPE = VT_BOOL;
            break;
        case BO_le:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("<= operator was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("le"));
            LAST_TYPE = VT_BOOL;
            break;
        case BO_gt:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR("> operator was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("gt"));
            LAST_TYPE = VT_BOOL;
            break;
        case BO_ge:
#ifdef DEBUGGING
            if (left_value == VT_BOOL) ERROR(">= operator was performed on boolean values");
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("ge"));
            LAST_TYPE = VT_BOOL;
            break;
        case BO_eq:
            instr = safe_concat_str(instr, STRcpy("eq"));
            LAST_TYPE = VT_BOOL;
            break;
        case BO_ne:
            instr = safe_concat_str(instr, STRcpy("ne"));
            LAST_TYPE = VT_BOOL;
            break;
        default:  // Should never happen
#ifdef DEBUGGING
            ERROR("Bytecode: Unexpected binop OP %i", BINOP_OP(node));
#endif // DEBUGGING
    }

    Instr(instr, NULL, NULL, NULL);

    MEMfree(instr);

    /**
     * Emit instruction for correct operator
     */
    return node;
}

/**
 * @fn BCmonop
 */
node_st *BCmonop(node_st *node)
{
    TRAVchildren(node);

    switch (MONOP_OP(node)) {
        case MO_neg: switch (LAST_TYPE) {
            case VT_NUM: Instr("ineg", NULL, NULL, NULL); break;
            case VT_FLOAT: Instr("fneg", NULL, NULL, NULL); break;
            default:
#ifdef DEBUGGING
                ERROR("Unexpected expression value %i for monop NEG", LAST_TYPE);
#endif // DEBUGGING
        } break;
        case MO_not:
            if (LAST_TYPE == VT_BOOL) Instr("bnot", NULL, NULL, NULL);
            else ERROR("Unexpected expression value %i for monop NOT", LAST_TYPE);
            break;
        default:  // Should never occur
#ifdef DEBUGGING
            ERROR("Unexpected monop OP %i", MONOP_OP(node));
#endif // DEBUGGING
    }

    // LAST_VALUE unchanged

    /**
     * Emit instruction for correct operator
     */
    return node;
}

/**
 * @fn BCvarlet
 */
node_st *BCvarlet(node_st *node)
{
    // Find symbol for varlet
    const Symbol* s = ScopeTreeFind(CURRENT_SCOPE, VARLET_NAME(node));
#ifdef DEBUGGING
    ASSERT_MSG((s != NULL), "BYTECODE: Could not find symbol named %s", VARLET_NAME(node));
#endif // DEBUGGING

    if (s->stype == ST_ARRAYVAR) {
        // Value should already be at stack

        // Push index onto stack (flatten multidim into single scalar)
        flatten_dim_exprs(s, VARLET_INDICES(node));

        // Push array reference onto stack
        load_array_ref(s);

        // Store value to index of array
        store_array_ref_with_value(s);

        // Return (don't run scalar instructions)
        return node;
    }

    const size_t current_level = CURRENT_SCOPE->nesting_level;
    const size_t var_level = s->parent_scope->nesting_level;
    const size_t var_offset = s->offset;

    char* instr = NULL;
    switch (s->vtype) {
        case VT_NUM: instr = STRcpy("i"); LAST_TYPE = VT_NUM; break;
        case VT_FLOAT: instr = STRcpy("f"); LAST_TYPE = VT_FLOAT; break;
        case VT_BOOL: instr = STRcpy("b"); LAST_TYPE = VT_BOOL; break;
        default:  // Should not occur
#ifdef DEBUGGING
            ERROR("Incompatible Varlet node with valuetype of %s", vt_to_str(s->vtype));
#endif // DEBUGGING
    }

    if (var_level == 0) {
        char* tmp_instr;
        if (s->imported) tmp_instr = STRcpy("storee");
        else tmp_instr = STRcpy("storeg");

        instr = safe_concat_str(instr, tmp_instr);
        char* var_offset_str = int_to_str((int) var_offset);
        Instr(instr, var_offset_str, NULL, NULL);
        MEMfree(var_offset_str);
    }

    else if (current_level == var_level) {
        instr = safe_concat_str(instr, STRcpy("store"));
        char* var_offset_str = int_to_str((int) var_offset);
        Instr(instr, var_offset_str, NULL, NULL);
        MEMfree(var_offset_str);
    }

    else {
#ifdef DEBUGGING
        ASSERT_MSG((current_level > var_level), "Calling variable from higher scope %lu compared to own scope %lu",
            var_level, current_level);
#endif // DEBUGGING
        instr = safe_concat_str(instr, STRcpy("storen"));
        char* var_delta_str = int_to_str((int) (current_level - var_level));
        char* offset_str = int_to_str((int) var_offset);
        Instr(instr, var_delta_str, offset_str, NULL);
        MEMfree(var_delta_str);
        MEMfree(offset_str);
    }

    MEMfree(instr);

    /**
     * Find which scope variable is from
     * Match:
     * --- Own scope: find index of variable in scope + save using "Store Local Variable" (1.4.5)
     * --- Higher frame: find index of variable in scope + save using "Store Relatively Free Variable" (1.4.5)
     * --- Global: find index of variable in scope + save using "Store Global Variable" (1.4.5)
     * --- Imported: find index of variable in scope + save using "Store Imported Variable" (1.4.5)
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCvar
 */
node_st *BCvar(node_st *node)
{
    // Retrieve var symbol from AST
    const Symbol* s = VAR_SYMBOL(node);
#ifdef DEBUGGING
    ASSERT_MSG((s != NULL), "BYTECODE: Could not find symbol named %s in function %s",
        VAR_NAME(node), CURRENT_SCOPE->parent_fun->name);
#endif // DEBUGGING

    if (s->stype == ST_ARRAYVAR) {
        if (VAR_INDICES(node) == NULL) {
            // If not indexed, push sizes to stack and then pointer
            push_array_with_dims(s);
            return node;
        }

        // If indexed, push value at index onto stack

        // Push index onto stack (flatten multidim into single scalar)
        flatten_dim_exprs(s, VAR_INDICES(node));

        // Push array reference onto stack
        load_array_ref(s);

        // Store value to index of array
        load_array_ref_with_value(s);

        // Return (don't run scalar instructions)
        return node;
    }

    const size_t current_level = CURRENT_SCOPE->nesting_level;
    const size_t var_level = s->parent_scope->nesting_level;
    const size_t var_offset = s->offset;

    char* instr = NULL;

    // Scalars
    if (!IS_ARRAY(s->vtype)) {
        switch (s->vtype) {
            case VT_NUM: instr = STRcpy("i"); LAST_TYPE = VT_NUM; break;
            case VT_FLOAT: instr = STRcpy("f"); LAST_TYPE = VT_FLOAT; break;
            case VT_BOOL: instr = STRcpy("b"); LAST_TYPE = VT_BOOL; break;
            default:  // Should not occur
#ifdef DEBUGGING
                ERROR("Incompatible Var node with valuetype of %s", vt_to_str(s->vtype));
#endif // DEBUGGING
        }

        if (var_level == 0) {
            if (s->imported) {
                instr = safe_concat_str(instr, STRcpy("loade"));
            } else {
                instr = safe_concat_str(instr, STRcpy("loadg"));
            }

            char* var_offset_str = int_to_str((int) var_offset);
            Instr(instr, var_offset_str, NULL, NULL);
            MEMfree(var_offset_str);
        }

        else if (current_level == var_level) {
            instr = safe_concat_str(instr, STRcpy("load"));
            if (var_offset <= 3) {
                switch (s->offset) {
                    case 0: instr = safe_concat_str(instr, STRcpy("_0")); break;
                    case 1: instr = safe_concat_str(instr, STRcpy("_1")); break;
                    case 2: instr = safe_concat_str(instr, STRcpy("_2")); break;
                    case 3: instr = safe_concat_str(instr, STRcpy("_3")); break;
                    default:  // Should never happen
#ifdef DEBUGGING
                        ERROR("Var with offset %lu was deemed to be within 0 and 3 inclusive", var_offset);
#endif // DEBUGGING
                }
                Instr(instr, NULL, NULL, NULL);
            } else {
                char* var_offset_str = int_to_str((int) var_offset);
                Instr(instr, var_offset_str, NULL, NULL);
                MEMfree(var_offset_str);
            }
        }

        else {
#ifdef DEBUGGING
            ASSERT_MSG((current_level > var_level), "Calling variable from higher scope %lu compared to own scope %lu",
                var_level, current_level);
#endif // DEBUGGING
            instr = safe_concat_str(instr, STRcpy("loadn"));
            char* delta_offset_str = int_to_str((int) (current_level - var_level));
            char* var_offset_str = int_to_str((int) var_offset);
            Instr(instr, delta_offset_str, var_offset_str, NULL);
            MEMfree(delta_offset_str);
            MEMfree(var_offset_str);
        }

        LAST_TYPE = s->vtype;
    }

    // Arrays
    else {
        // Load int values of array dims and push array reference itself
        push_array_with_dims(s);

        LAST_TYPE = s->vtype;
    }

    MEMfree(instr);

    HAD_EXPR = true;

    /**
     * Find which scope variable is from
     * Match:
     * --- Own scope: find index of variable in scope + load using "Load Local Variable" (1.4.5)
     * --- Higher frame: find index of variable in scope + load using "Load Relatively Free Variable" (1.4.5)
     * --- Global: find index of variable in scope + load using "Load Global Variable" (1.4.5)
     * --- Imported: find index of variable in scope + load using "Load Imported Variable" (1.4.5)
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCnum
 */
node_st *BCnum(node_st *node)
{
    TRAVchildren(node);

    /**
     * Emit constant
     * Emit instruction to load constant
     */

    const int v = NUM_VAL(node);
    switch (v) {
        case -1: Instr("iloadc_m1", NULL, NULL, NULL); break;
        case 0: Instr("iloadc_0", NULL, NULL, NULL); break;
        case 1: Instr("iloadc_1", NULL, NULL, NULL); break;
        default: ;  // Don't remove this semicolon, it's here because a statement is expected
                    // and the declaration after is not a statement so the semicolon serves
                    // as an empty statement :)
            char* val_str = int_to_str(v);

            // Refer to existing constant if possible
            const ConstEntry res = ASMfindConstant(&ASM, val_str);

            char* const_count_str;
            if (res.get != NULL) {
                const_count_str = int_to_str((int) res.offset);
            } else {
                ASMemitConst(&ASM, "int", val_str);
                const_count_str = int_to_str((int) CONST_COUNT++);
            }

            Instr("iloadc", const_count_str, NULL, NULL);

            MEMfree(val_str);
            MEMfree(const_count_str);
            break;
    }

    LAST_TYPE = VT_NUM;
    HAD_EXPR = true;

    return node;
}

/**
 * @fn BCfloat
 */
node_st *BCfloat(node_st *node)
{
    TRAVchildren(node);

    /**
     * Emit constant
     * Emit instruction to load constant
     */

    const float v = FLOAT_VAL(node);
    if (v == 0.0) Instr("floadc_0", NULL, NULL, NULL);
    else if (v == 1.0) Instr("floadc_1", NULL, NULL, NULL);
    else {
        char* val_str = float_to_str(v);

        // Refer to existing constant if possible
        const ConstEntry res = ASMfindConstant(&ASM, val_str);

        char* const_count_str;
        if (res.get != NULL) {
           const_count_str = int_to_str((int) res.offset);
        } else {
            ASMemitConst(&ASM, "float", val_str);
            const_count_str = int_to_str((int) CONST_COUNT++);
        }

        Instr("floadc", const_count_str, NULL, NULL);

        MEMfree(val_str);
        MEMfree(const_count_str);
    }

    LAST_TYPE = VT_FLOAT;
    HAD_EXPR = true;

    return node;
}

/**
 * @fn BCbool
 */
node_st *BCbool(node_st *node)
{
    TRAVchildren(node);

    /**
     * Emit constant
     * Emit instruction to load constant
     */

    if (BOOL_VAL(node) == true) {
        Instr("bloadc_t", NULL, NULL, NULL);
    } else {
        Instr("bloadc_f", NULL, NULL, NULL);
    }

    LAST_TYPE = VT_BOOL;
    HAD_EXPR = true;

    return node;
}

