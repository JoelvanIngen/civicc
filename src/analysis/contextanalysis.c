/**
 * @file
 *
 * Traversal: ContextAnalysis
 * UID      : CTA
 *
 *
 */

// TODO: Find method to count amount of array init entries,
// and push scope's offset counter by that amount

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

#include "common.h"
#include "global/globals.h"
#include "idlist.h"
#include "argstack.h"
#include "dimsliststack.h"
#include "symbol/scopetree.h"
#include "symbol/table.h"

typedef enum {
    DECLARATION_PASS,
    ANALYSIS_PASS,
} PassType;

static PassType PASS;
static ValueType LAST_TYPE;

// Current scope in the tree
static SymbolTable* CURRENT_SCOPE;

// Stacking nested function calls
static ArgListStack* ALS;

// Stacking nested array indexing
static DimsListStack* DLS;

// Flag to determine if we are saving indexes to stack
static bool SAVING_IDXS = false;

// Listing parameter dimensions
static IdList* IDL = NULL;

// Keeps track of whether we had an error during analysis
static bool HAD_ERROR = false;

#define IS_ARITH_TYPE(vt) (vt == VT_NUM || vt == VT_FLOAT)
#define IS_ARRAY(vt) (vt == VT_NUMARRAY || vt == VT_FLOATARRAY || vt == VT_BOOLARRAY)

// Short for printing error on duplicate identifier name and immediately returning
#define HANDLE_DUPLICATE_ID(name) do { \
    if (name_exists_in_top_scope(name)) { \
        HAD_ERROR = true; \
        USER_ERROR("Variable '%s' is already declared", name); \
        return node; \
    } \
} while (false)

#define HANDLE_MISSING_SYMBOL(name, s) do { \
    if (s == NULL) { \
        HAD_ERROR = true; \
        USER_ERROR("Variable '%s' has not been declared yet", name); \
        return node; \
    } \
} while (false)



ValueType demote_array_type(const ValueType array_type) {
    switch (array_type) {
        case VT_NUMARRAY: return VT_NUM;
        case VT_FLOATARRAY: return VT_FLOAT;
        case VT_BOOLARRAY: return VT_BOOL;
        default: // Should never occur
#ifdef DEBUGGING
            ERROR("Cannot demote ValueType array %i", array_type);
#endif
    }
}

static void exit_if_error() {
    if (HAD_ERROR) {
        USER_ERROR("One or multiple errors occurred, exiting...");
        exit(1);
    }
}

static bool name_exists_in_top_scope(char* name) {
    return STlookup(CURRENT_SCOPE, name) != NULL;
}

/**
 * Traverses and saves the indices to DimsList. This function is for
 * nodes that use the 'indices' macro. DLSpop needs to be called after
 * this function, if the values are needed before discarding
 * @param node current node
 */
static void handle_array_dims_exprs_for_indices_macro(node_st* node) {
    // Push new DimList onto the stack
    DLSpush(DLS);

    // Get old flag for saving indices
    const bool was_saving_idxs = SAVING_IDXS;

    // Set new flag for saving indices to true
    SAVING_IDXS = true;

    TRAVindices(node);

    // Restore saving indices flag
    SAVING_IDXS = was_saving_idxs;
}

/**
 * Traverses and saves the indices to DimsList. This function is for
 * nodes that use the 'dims' macro. DLSpop needs to be called after
 * this function, if the values are needed before discarding
 * @param node current node
 */
static void handle_array_dims_exprs_for_dims_macro(node_st* node) {
    // Push new DimList onto the stack
    DLSpush(DLS);

    // Get old flag for saving indices
    const bool was_saving_idxs = SAVING_IDXS;

    // Set new flag for saving indices to true
    SAVING_IDXS = true;

    TRAVdims(node);

    // Restore saving indices flag
    SAVING_IDXS = was_saving_idxs;
}

void CTAinit() {  }
void CTAfini() {
    // Free functions also delete any leftovers, we don't worry about cleaning up
    ALSfree(&ALS);
    DLSfree(&DLS);
}

/**
 * @fn CTAprogram
 */
node_st *CTAprogram(node_st *node)
{
    // TODO: Free global scope after bytecode generation
    CURRENT_SCOPE = STnew(NULL, NULL);
    GB_GLOBAL_SCOPE = CURRENT_SCOPE;

    // Initialise funcall argument stack
    ALS = ALSnew();

    // Initialise dimension stack
    DLS = DLSnew();

    /* First pass; we only look at function definitions, so we can correctly
     * handle usages of functions that have not been defined yet */
    PASS = DECLARATION_PASS;
    TRAVchildren(node);

    /* Second pass; we perform full analysis of the body now that we know
     * all definitions */
    PASS = ANALYSIS_PASS;
    TRAVchildren(node);

    // Exit here if an analysis error occurred
    exit_if_error();

    return node;
}

/**
 * @fn CTAdecls
 */
node_st *CTAdecls(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAexprs
 */
node_st *CTAexprs(node_st *node)
{
    // Keep state of index keeping and set to false to prevent children from adding themselves
    const bool was_saving_idxs = SAVING_IDXS;

    SAVING_IDXS = false;
    TRAVexpr(node);
    SAVING_IDXS = was_saving_idxs;

    if (SAVING_IDXS) {
        // TODO: Find out which values are allowed (can floats be used for index, does bool cast to idx 0 or 1)?
        if (LAST_TYPE != VT_NUM) {
            HAD_ERROR = true;
            USER_ERROR("Can only index arrays with integers");
        }

        // TODO: this doesn't really make sense, we don't know the value yet, so
        // we will need to rework this, but in the meantime, do add it to DimsList
        // since we do want to keep the amount of dimensions for analysis
        // We'll just use 0 until we rework this
        DLSadd(DLS, 0);
    }

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAarrexpr
 */
node_st *CTAarrexpr(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAids
 */
node_st *CTAids(node_st *node)
{
    char* name = IDS_NAME(node);

    // Add ID name to ID list
    IDLadd(IDL, name);

    // Add name to scope as type integer
    STinsert(CURRENT_SCOPE, name, VT_NUM);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAexprstmt
 */
node_st *CTAexprstmt(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAreturn
 */
node_st *CTAreturn(node_st *node)
{
    TRAVchildren(node);

    // Find type of return expression
    const node_st* expr = RETURN_EXPR(node);
    const ValueType ret_type = expr == NULL ? VT_VOID : LAST_TYPE;

    // Check validity of return expression
    const ValueType parent_fun_type = CURRENT_SCOPE->parent_fun->vtype;
    if (ret_type != parent_fun_type) {
        USER_ERROR("Trying to return %s from function with type %s",
            vt_to_str(ret_type), vt_to_str(parent_fun_type));
    }

    return node;
}

/**
 * @fn CTAfuncall
 */
node_st *CTAfuncall(node_st *node)
{
    // Check if function exists and is actually a function
    char* name = FUNCALL_NAME(node);
    Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);
    if (!s) {
        HAD_ERROR = true;
        USER_ERROR("Function name %s does not exist in scope.", name);
        LAST_TYPE = VT_VOID;
        return node;
    }

    if (s->stype != ST_FUNCTION) {
        HAD_ERROR = true;
        USER_ERROR("Name %s is not defined as a function.", name);
        LAST_TYPE = VT_VOID;
        return node;
    }

    // Push new function call stack to allow for nested function calls
    ALSpush(ALS);
    TRAVchildren(node);

    // Arguments provided in function call
    Argument** args = ALSgetCurrentArgs(ALS);
    const size_t args_len = ALSgetCurrentLength(ALS);

    // Arguments expected by function
    const ValueType* param_types = s->as.fun.param_types;
    const size_t* param_dim_counts = s->as.fun.param_dim_counts;
    const size_t params_len = s->as.fun.param_count;

    // Error if more arguments provided than parameters expected
    if (args_len > params_len) {
        USER_ERROR("Too many arguments; got %lu but function %s only expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
        return node;
    }

    // Error if more parameters expected than arguments provided
    if (args_len < params_len) {
        USER_ERROR("Not enough arguments; got %lu but function %s expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
        return node;
    }

    // Iterate through all params and arguments and compare properties
    for (size_t i = 0; i < params_len; i++) {
        const Argument* arg = args[i];
        // Error if not similar type
        if (arg->type != param_types[i]) {
            USER_ERROR("Argument type %s and parameter type %s don't match",
                vt_to_str(arg->type), vt_to_str(param_types[i]));
            return node;
        }

        if (IS_ARRAY(arg->type)) {
            // Compare argument dimensions vs expected parameter dimensions
            if (arg->arr_dim_count != param_dim_counts[i]) {
                USER_ERROR("Argument has %lu dimensions, but function parameter expects %lu",
                    arg->arr_dim_count, param_dim_counts[i]);
            }
        }
    }

    // Remove function call from stack
    ALSpop(ALS);

    // Last type is function return type
    LAST_TYPE = s->vtype;
    return node;
}

/**
 * @fn CTAcast
 */
node_st *CTAcast(node_st *node)
{
    TRAVchildren(node);
    if (!(LAST_TYPE == VT_NUM || LAST_TYPE == VT_FLOAT || LAST_TYPE == VT_BOOL)) {
        USER_ERROR("Invalid cast source, can only cast from Num, Float or Bool but got %s",
            vt_to_str(LAST_TYPE));
    }

    LAST_TYPE = ct_to_vt(CAST_TYPE(node), false);
    return node;
}

/**
 * @fn CTAfundefs
 */
node_st *CTAfundefs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAfundef
 */
node_st *CTAfundef(node_st *node)
{
    char* fun_name = FUNDEF_NAME(node);
    const ValueType ret_type = ct_to_vt(FUNDEF_TYPE(node), false);

    if (PASS == DECLARATION_PASS) {
        /* First pass: only return information about self */
        const bool is_extern = FUNDEF_IS_EXTERN(node);
        Symbol* s = SBfromFun(fun_name, ret_type, is_extern);

        if (!is_extern) {
            s->as.fun.scope = STnew(CURRENT_SCOPE, s);
        }

        STinsert(CURRENT_SCOPE, fun_name, s);
    } else {
        /* Second pass: explore information about own statements
         * Here we also detect if variables are wrongly typed */

        const Symbol* s = STlookup(CURRENT_SCOPE, fun_name);

        // Only explore if not extern
        if (!s->imported) {
            // Switch to new scope
            CURRENT_SCOPE = s->as.fun.scope;

            // Add parameters to scope
            TRAVparams(node);

            // Explore function body
            TRAVbody(node);

            // Reset loop counter
            CURRENT_SCOPE->for_loop_counter = 0;

            // Switch back to parent scope
            CURRENT_SCOPE = CURRENT_SCOPE->parent_scope;
        }
    }

    return node;
}

/**
 * @fn CTAfunbody
 */
node_st *CTAfunbody(node_st *node)
{
    // Explore inner functions and declarations that might be used by inner statements
    PASS = DECLARATION_PASS;
    TRAVdecls(node);
    TRAVlocal_fundefs(node);

    // Explore local functions and own statements now that we gathered all declarations
    PASS = ANALYSIS_PASS;
    TRAVlocal_fundefs(node);
    TRAVstmts(node);
    return node;
}

/**
 * @fn CTAifelse
 */
node_st *CTAifelse(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAwhile
 */
node_st *CTAwhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAdowhile
 */
node_st *CTAdowhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAfor
 */
node_st *CTAfor(node_st *node)
{
    char* name = FOR_VAR(node);
    char* adjusted_name = safe_concat_str(
        int_to_str((int) CURRENT_SCOPE->for_loop_counter),
        safe_concat_str(STRcpy("_"), STRcpy(name)));

    // Create for-loop entry in current scope
    Symbol* s_loop = SBfromForLoop(adjusted_name);
    STinsert(CURRENT_SCOPE, adjusted_name, s_loop);

    // Create for-loop scope and switch to it
    s_loop->as.forloop.scope = STnew(CURRENT_SCOPE, s_loop);
    CURRENT_SCOPE = s_loop->as.forloop.scope;

    // Create loop variable in new scope
    Symbol* s_var = SBfromVar(name, VT_NUM, false);
    STinsert(CURRENT_SCOPE, name, s_var);

    TRAVchildren(node);

    // Special case: restore loop counter to zero for next traversal (during bytecode)
    CURRENT_SCOPE->for_loop_counter = 0;

    // Restore scope
    CURRENT_SCOPE = CURRENT_SCOPE->parent_scope;

    // Increment loop counter for next for-loop
    CURRENT_SCOPE->for_loop_counter++;

    // Clean up
    MEMfree(adjusted_name);

    return node;
}

/**
 * @fn CTAglobdecl
 */
node_st *CTAglobdecl(node_st *node)
{
    // Note: Requires array support

    // Immediate exit if phase is not DECLARATION_PASS to prevent adding twice
    if (PASS != DECLARATION_PASS) return node;

    char* name = GLOBDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions, represented as IDs
    IDL = IDLnew();
    TRAVdims(node);

    const bool is_array = IDL->ptr > 0;

    // Add self to vartable of current scope
    Symbol* s;
    const ValueType type = ct_to_vt(GLOBDECL_TYPE(node), is_array);

    if (is_array) {
        s = SBfromArray(name, type, true);
        s->as.array.dim_count = IDL->ptr;
    } else {
        s = SBfromVar(name, type, true);
    }

    STinsert(CURRENT_SCOPE, name, s);

    // Clean up - note: free function does not (yet) free internal ids list
    IDLfree(&IDL);

    return node;
}

/**
 * @fn CTAglobdef
 */
node_st *CTAglobdef(node_st *node)
{
    // Note: Requires array support

    // Immediate exit if phase is not DECLARATION_PASS to prevent adding twice
    if (PASS != DECLARATION_PASS) return node;

    char* name = GLOBDEF_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions
    handle_array_dims_exprs_for_dims_macro(node);

    // Add self to vartable of current scope
    const ValueType type = ct_to_vt(GLOBDEF_TYPE(node), false);

    Symbol* s = SBfromVar(name, type, false);
    STinsert(CURRENT_SCOPE, name, s);

    // Save information to array symbol
    DimsList* dml = DLSpeekTop(DLS);

    bool is_array = false;
    if (dml->size > 0) {
        is_array = true;
        s->as.array.dim_count = dml->size;
        s->as.array.dims = dml->dims;
    }

    // Clean up, except for dims list
    DMLfree(&dml);

    TRAVinit(node);

    // TODO: Check if types implicitly cast
    // We can use last_type because globdef always has an init child
    if (ct_to_vt(GLOBDEF_TYPE(node), is_array) != LAST_TYPE) {
        USER_ERROR("Variable of type %s was initialised with expression of type %s",
            vt_to_str(ct_to_vt(GLOBDEF_TYPE(node), is_array)),
            vt_to_str(LAST_TYPE));
    }

    // TODO: Allow scalar to init array

    return node;
}

/**
 * @fn CTAparam
 */
node_st *CTAparam(node_st *node)
{
    // Note: Requires array support

    char* param_name = PARAM_NAME(node);

    HANDLE_DUPLICATE_ID(param_name);

    // Find dimensions, represented as IDs
    IDL = IDLnew();
    TRAVdims(node);

    // Is pointer if more than zero
    const bool is_array = IDL->ptr > 0;

    // Add self to vartable of current scope
    const ValueType param_type = ct_to_vt(PARAM_TYPE(node), is_array);

    // Add parameter as variable to scope
    // TODO: Verify that param identifier cannot be imported (unless size is specified externally)
    Symbol* s = SBfromVar(param_name, param_type, false);
    STinsert(CURRENT_SCOPE, param_name, s);
    s->offset = CURRENT_SCOPE->parent_fun->offset++;

    if (is_array) {
        // Add array properties
        s->as.array.dim_count = IDL->ptr;
    }

    // Add parameter to the function it belongs to
    Symbol* parent_fun = CURRENT_SCOPE->parent_fun;
    SBaddParam(parent_fun, param_type, IDL->ptr);

    IDLfree(&IDL);

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAvardecl
 */
node_st *CTAvardecl(node_st *node)
{
    // Note: Requires array support

    char* name = VARDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions
    handle_array_dims_exprs_for_dims_macro(node);

    const DimsList* dml = DLSpeekTop(DLS);
    const bool is_array = dml->size > 0;

    const ValueType type = ct_to_vt(VARDECL_TYPE(node), is_array);

    // Create and add symbol to scope
    Symbol* s = SBfromVar(name, type, false);
    STinsert(CURRENT_SCOPE, name, s);
    s->offset = CURRENT_SCOPE->parent_fun->offset++;

    if (is_array) {
        // Save information to array symbol
        s->as.array.dim_count = dml->size;
        s->as.array.dims = dml->dims;
    }

    // Clean up
    DLSpop(DLS);

    LAST_TYPE = VT_NULL;
    TRAVinit(node);

    // Find out if an init existed
    if (LAST_TYPE != VT_NULL) {
        // Compare types
        // TODO: Allow scalar to init array
        // TODO: Find out if types implicitly cast
        if (s->vtype != LAST_TYPE) {
            USER_ERROR("Tried to initialise variable %s with %s",
                vt_to_str(s->vtype), vt_to_str(LAST_TYPE));
        }
    }

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAstmts
 */
node_st *CTAstmts(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAassign
 */
node_st *CTAassign(node_st *node)
{
    // Note: Might require array support?

    TRAVlet(node);
    const ValueType let_type = LAST_TYPE;
    const bool let_is_arith = IS_ARITH_TYPE(let_type);

    TRAVchildren(node);
    const ValueType expr_type = LAST_TYPE;
    const bool expr_is_arith = IS_ARITH_TYPE(expr_type);

    // Numbers are compatible
    if (let_is_arith && expr_is_arith) {
        // If types are both arith but not same, implicit conversion takes place
        if (let_type == VT_NUM && expr_type == VT_FLOAT) {
            ASSIGN_EXPR(node) = ASTcast(ASSIGN_EXPR(node), CT_int);
        } else if (let_type == VT_FLOAT && expr_type == VT_NUM) {
            ASSIGN_EXPR(node) = ASTcast(ASSIGN_EXPR(node), CT_float);
        }
        LAST_TYPE = let_type == VT_FLOAT || expr_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (let_type == VT_BOOL && expr_type == VT_BOOL) {
        LAST_TYPE = VT_BOOL;
    }

    else {
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_str(let_type), vt_to_str(expr_type));
    }

    return node;
}

/**
 * @fn CTAbinop
 */
node_st *CTAbinop(node_st *node)
{
    TRAVleft(node);
    const ValueType left_type = LAST_TYPE;
    const bool left_is_arith = IS_ARITH_TYPE(left_type);

    TRAVright(node);
    const ValueType right_type = LAST_TYPE;
    const bool right_is_arith = IS_ARITH_TYPE(right_type);

    // Numbers are compatible
    if (left_is_arith && right_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        if (left_type == VT_NUM && right_type == VT_FLOAT) {
            BINOP_LEFT(node) = ASTcast(BINOP_LEFT(node), CT_float);
        } else if (left_type == VT_FLOAT && right_type == VT_NUM) {
            BINOP_RIGHT(node) = ASTcast(BINOP_RIGHT(node), CT_float);
        }

        LAST_TYPE = left_type == VT_FLOAT || right_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (left_type == VT_BOOL && right_type == VT_BOOL) {
        LAST_TYPE = VT_BOOL;
    }

    else {
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_str(left_type), vt_to_str(right_type));
    }

    // Check individual operators with types
    const enum BinOpType t = BINOP_OP(node);
    if (left_is_arith) {
        if (!(t == BO_add || t == BO_sub || t == BO_mul || t == BO_div || t == BO_mod ||
              t == BO_eq || t == BO_ne || t == BO_lt || t == BO_le || t == BO_gt || t == BO_ge)) {
            USER_ERROR("Invalid operands %s and %s for operator %s",
                vt_to_str(left_type), vt_to_str(right_type), bo_to_str(t));
        }

        // If equality operator, LAST_TYPE should become boolean
        if (t == BO_eq || t == BO_ne || t == BO_lt || t == BO_le || t == BO_gt || t == BO_ge) {
            LAST_TYPE = VT_BOOL;
        }
    } else if (left_type == VT_BOOL) {
        // BO_add and BO_mul are strict logic disjunction and conjunction, respectively
        if (!(t == BO_add || t == BO_mul || t == BO_eq || t == BO_ne || t == BO_and || t == BO_or)) {
            USER_ERROR("Invalid operands %s and %s for operator %s",
                vt_to_str(left_type), vt_to_str(right_type), bo_to_str(t));
        }
        LAST_TYPE = VT_BOOL;
    } else {
#ifdef DEBUGGING
        ERROR("Unexpected operand %s is not arith or bool", vt_to_str(left_type));
#endif // DEBUGGING
    }

    return node;
}

/**
 * @fn CTAmonop
 */
node_st *CTAmonop(node_st *node)
{
    TRAVoperand(node);

    // Confirm type is correct
    switch (MONOP_OP(node)) {
        // TODO: Find out if negated bool switches values (probably not)
        case MO_neg:
            if (!(LAST_TYPE == VT_NUM || LAST_TYPE == VT_FLOAT)) {
                USER_ERROR("Expected operand type Int or Float, got %s instead", vt_to_str(LAST_TYPE));
            }
            break;
        case MO_not: if (LAST_TYPE != VT_BOOL) {
                USER_ERROR("Expected operand type Bool, got %s instead", vt_to_str(LAST_TYPE));
            }
            break;
        default: /* Should never occur */
            USER_ERROR("Unknown monop %s with operand %s", mo_to_str(MONOP_OP(node)), vt_to_str(LAST_TYPE));
    }

    return node;
}

/**
 * @fn CTAvarlet
 */
node_st *CTAvarlet(node_st *node)
{
    // TODO: Disallow storing to imported variables

    // Note: Requires array support

    char* name = VARLET_NAME(node);

    // Look up variable
    const Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);

    // Handle case of missing symbol
    HANDLE_MISSING_SYMBOL(name, s);

    // Find dimensions
    handle_array_dims_exprs_for_indices_macro(node);

    const DimsList* dml = DLSpeekTop(DLS);
    const bool is_array = dml->size > 0;

    if (is_array) {
        // Ensure symbol is array
        if (!IS_ARRAY(s->vtype)) {
            USER_ERROR("Identifier %s was indexed, but is not an array", name);
        }

        // Confirm dimensions are correct
        if (s->as.array.dim_count != dml->size) {
            USER_ERROR("Expected array dimension count is %lu but got %lu", s->as.array.dim_count, dml->size);
        }

        LAST_TYPE = demote_array_type(s->vtype);
    } else {
        LAST_TYPE = s->vtype;
    }

    // Clean up
    DLSpop(DLS);

    return node;
}

/**
 * @fn CTAvar
 */
node_st *CTAvar(node_st *node)
{
    // Note: Requires array support

    char* name = VAR_NAME(node);

    // Look up variable
    const Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);

    // Handle case of missing symbol
    HANDLE_MISSING_SYMBOL(name, s);

    // Find dimensions
    handle_array_dims_exprs_for_indices_macro(node);

    const DimsList* dml = DLSpeekTop(DLS);
    const bool is_array = dml->size > 0;

    if (is_array) {
        // Ensure symbol is array
        if (!IS_ARRAY(s->vtype)) {
            USER_ERROR("Identifier %s was indexed, but is not an array", name);
        }

        // Confirm dimensions are correct
        if (s->as.array.dim_count != dml->size) {
            USER_ERROR("Expected array dimension count is %lu but got %lu", s->as.array.dim_count, dml->size);
        }

        LAST_TYPE = demote_array_type(s->vtype);
    } else {
        LAST_TYPE = s->vtype;
    }

    // Clean up
    DLSpop(DLS);

    return node;
}

/**
 * @fn CTAnum
 */
node_st *CTAnum(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_NUM;
    return node;
}

/**
 * @fn CTAfloat
 */
node_st *CTAfloat(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_FLOAT;
    return node;
}

/**
 * @fn CTAbool
 */
node_st *CTAbool(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_BOOL;
    return node;
}

