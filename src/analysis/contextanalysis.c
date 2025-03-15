/**
 * @file
 *
 * Traversal: ContextAnalysis
 * UID      : CTA
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

#include "common.h"
#include "idlist.h"
#include "argstack.h"
#include "dimsliststack.h"
#include "symbol/tablestack.h"

char* VT_TO_STR[] = {"int", "float", "bool", "void", "int[]", "float[]", "bool[]"};

typedef enum {
    DECLARATION_PASS,
    ANALYSIS_PASS,
} PassType;

PassType PASS;
ValueType last_type;

// Stacking nested functions
SymbolTableStack* STS;

// Stacking nested function calls
ArgListStack* ALS;

// Stacking nested array indexing
DimsListStack* DLS;

// Listing parameter dimensions
IdList* IDL = NULL;

// Keeps track of whether we had an error during analysis
bool HAD_ERROR = false;

#define IS_ARITH_TYPE(vt) (vt == VT_NUM || vt == VT_FLOAT)

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

ValueType valuetype_from_nt(const enum Type ct_type, const bool is_array) {
    switch (ct_type) {
        case CT_int: return is_array ? VT_NUMARRAY : VT_NUM;
        case CT_float: return is_array ? VT_FLOATARRAY : VT_FLOAT;
        case CT_bool: return is_array ? VT_BOOLARRAY : VT_BOOL;
        case CT_void:
            if (is_array) {
                // Doesn't exist - TODO: find out if actually doesn't exist
                printf("Type error: cannot have a void array\n");
                exit(1);
            }
        return VT_VOID;
        default:
            printf("Type error: unexpected ct_type %i\n", ct_type);
        exit(1);
    }
}

char* vt_to_string(const ValueType vt) {
#ifdef DEBUGGING
    ASSERT_MSG((vt >= 0 && vt < 7), "Valuetype enum out of range: %i", vt);
#endif
    return VT_TO_STR[vt];
}

static void exit_if_error() {
    if (HAD_ERROR) {
        USER_ERROR("One or multiple errors occurred, exiting...");
        exit(1);
    }
}

static bool name_exists_in_top_scope(char* name) {
    return STSlookup(STS, name) != NULL;
}

void CTAinit() { }
void CTAfini() {
    // Free functions also delete any leftovers, we don't worry about cleaning up
    STSfree(&STS);
    ALSfree(&ALS);
    DLSfree(&DLS);
}

/**
 * @fn CTAprogram
 */
node_st *CTAprogram(node_st *node)
{
    // Initialise symbol table stack
    STS = STSnew();

    // Initialise funcall argument stack
    ALS = ALSnew();

    // Initialise dimension stack
    DLS = DLSnew();

    // Create global scope, with void return type (will never be used anyway)
    STSpush(STS, NULL, VT_VOID);

    /* First pass; we only look at function definitions, so we can correctly
     * handle usages of functions that have not been defined yet
     */
    PASS = DECLARATION_PASS;
    TRAVchildren(node);

    /* Second pass; we perform full analysis of the body now that we know
     * all definitions
     */
    PASS = ANALYSIS_PASS;
    TRAVchildren(node);

    // If we had an error, exit
    exit_if_error();

    return node;
}

/**
 * @fn CTAdecls
 */
node_st *CTAdecls(node_st *node)
{
    // Reset error flag
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAexprs
 */
node_st *CTAexprs(node_st *node)
{
    TRAVchildren(node);
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
    const ValueType ret_type = expr == NULL ? VT_VOID : last_type;

    // Check validity of return expression
    char* parent_fun_name = STScurrentScopeName(STS);
    const Symbol* parent_fun = STSlookup(STS, parent_fun_name);
    if (ret_type != parent_fun->vtype) {
        USER_ERROR("Trying to return %s from function with type %s",
            vt_to_string(ret_type), vt_to_string(parent_fun->vtype));
    }

    return node;
}

/**
 * @fn CTAfuncall
 */
node_st *CTAfuncall(node_st *node)
{
    // Check if function exists and is actually a function
    Symbol* s = STSlookup(STS, FUNCALL_NAME(node));
    if (!s) {
        HAD_ERROR = true;
        USER_ERROR("Function name %s does not exist in scope.", FUNCALL_NAME(node));
        last_type = VT_VOID;
        return node;
    }

    if (s->stype != ST_FUNCTION) {
        HAD_ERROR = true;
        USER_ERROR("Name %s is not defined as a function.", FUNCALL_NAME(node));
        last_type = VT_VOID;
        return node;
    }

    // Push new function call stack to allow for nested function calls
    ALSpush(ALS);
    TRAVchildren(node);

    const Argument* args = ALSgetCurrentArgs(ALS);
    const size_t args_len = ALSgetCurrentLength(ALS);

    const ValueType* param_types = s->as.fun.param_types;
    const size_t params_len = s->as.fun.param_count;

    if (args_len > params_len) {
        USER_ERROR("Too many arguments; got %lu but function %s only expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
    }

    if (args_len < params_len) {
        USER_ERROR("Not enough arguments; got %lu but function %s expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
    }

    for (size_t i = 0; i < params_len; i++) {
        if (args->type != param_types[i]) {
            USER_ERROR("Argument type %s and parameter type %s don't match",
                vt_to_string(args->type), vt_to_string(param_types[i]));
        }

        // TODO: Compare array dimensions
    }

    // Remove function call from argstack
    ALSpop(ALS);

    // Last type is function return type
    last_type = s->vtype;
    return node;
}

/**
 * @fn CTAcast
 */
node_st *CTAcast(node_st *node)
{
    TRAVchildren(node);
    if (!(last_type == VT_NUM || last_type == VT_FLOAT || last_type == VT_BOOL)) {
        USER_ERROR("Invalid cast source, can only cast from Num, Float or Bool but got %s",
            vt_to_string(last_type));
    }

    last_type = valuetype_from_nt(CAST_TYPE(node), false);
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
    const ValueType ret_type = valuetype_from_nt(FUNDEF_TYPE(node), false);

    if (PASS == DECLARATION_PASS) {
        /* First pass: only return information about self */
        Symbol* s = SBfromFun(fun_name, ret_type);
        STSadd(STS, fun_name, s);
    } else {
        /* Second pass: explore information about own statements
         * Here we also detect if variables are wrongly typed */

        // Add local scope to stack
        STSpush(STS, fun_name, ret_type);

        // Add parameters to scope
        TRAVparams(node);

        // Find definitions in function body
        PASS = DECLARATION_PASS;
        TRAVbody(node);

        // Explore function body
        PASS = ANALYSIS_PASS;
        TRAVbody(node);

        // Discard scope as it is no longer needed
        STSpop(STS);
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
    // Create new scope to ensure proper handling of loop variable
    STSpush(STS, NULL, VT_VOID);

    char* name = FOR_VAR(node);

    Symbol* s = SBfromVar(name, VT_NUM);
    STSadd(STS, name, s);

    TRAVchildren(node);

    // Discard scope
    STSpop(STS);

    return node;
}

/**
 * @fn CTAglobdecl
 */
node_st *CTAglobdecl(node_st *node)
{
    char* name = GLOBDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Add self to vartable of current scope
    const ValueType type = valuetype_from_nt(GLOBDECL_TYPE(node), false);

    Symbol* s = SBfromVar(name, type);
    STSadd(STS, name, s);

    // TODO: Not actually save to array.dims but assign to variables
    // Create IdList to traverse dimensions
    // IDL = IDLnew();
    TRAVchildren(node);

    // s->as.array.dim_count = IDL->size;
    // s->as.array.dims = IDL->ids;

    // // Clean up, except for dims list
    // IDLfree(&IDL);

    return node;
}

/**
 * @fn CTAglobdef
 */
node_st *CTAglobdef(node_st *node)
{
    // TODO: Add array support

    char* name = GLOBDEF_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Add self to vartable of current scope
    const ValueType type = valuetype_from_nt(GLOBDEF_TYPE(node), false);

    Symbol* s = SBfromVar(name, type);
    STSadd(STS, name, s);

    // Create new entry on indexing stack
    DLSpush(DLS);

    // TODO: activate and deactivate saving to DLS based on global parameter
    // SAVE_DIMS = true;
    TRAVdims(node);
    // SAVE_DIMS = false;

    // Save information to array symbol
    DimsList* dml = DLSpeekTop(DLS);
    s->as.array.dim_count = dml->size;
    s->as.array.dims = dml->dims;

    // Clean up, except for dims list
    DMLfree(&dml);

    TRAVinit(node);
    return node;
}

/**
 * @fn CTAparam
 */
node_st *CTAparam(node_st *node)
{
    // TODO: Add array support

    char* param_name = PARAM_NAME(node);

    HANDLE_DUPLICATE_ID(param_name);

    // Add self to vartable of current scope
    const ValueType param_type = valuetype_from_nt(PARAM_TYPE(node), false);

    // Add parameter as variable to scope
    Symbol* s = SBfromVar(param_name, param_type);
    STSadd(STS, param_name, s);

    // Add parameter to the function it belongs to
    char* fun_name = STScurrentScopeName(STS);
    Symbol* parent_fun = STSlookup(STS, fun_name);
    SBaddParam(parent_fun, param_type);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAvardecl
 */
node_st *CTAvardecl(node_st *node)
{
    // TODO: Add array support

    char* name = VARDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Add self to vartable of current scope
    const ValueType type = valuetype_from_nt(VARDECL_TYPE(node), false);

    Symbol* s = SBfromVar(name, type);
    STSadd(STS, name, s);

    // Create new entry on indexing stack
    DLSpush(DLS);

    // TODO: activate and deactivate saving to DLS based on global parameter
    // SAVE_DIMS = true;
    TRAVdims(node);
    // SAVE_DIMS = false;

    // Save information to array symbol
    DimsList* dml = DLSpeekTop(DLS);
    s->as.array.dim_count = dml->size;
    s->as.array.dims = dml->dims;

    // Clean up, except for dims list
    DMLfree(&dml);

    TRAVinit(node);
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
    // TODO: Add array support

    TRAVlet(node);
    const ValueType let_type = last_type;
    const bool let_is_arith = IS_ARITH_TYPE(let_type);

    TRAVchildren(node);
    const ValueType expr_type = last_type;
    const bool expr_is_arith = IS_ARITH_TYPE(expr_type);

    // Numbers are compatible, although implicit casting will take place
    if (let_is_arith && expr_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        last_type = let_type == VT_FLOAT || expr_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (let_type == VT_BOOL && expr_type == VT_BOOL) {
        last_type = VT_BOOL;
    }

    else {
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_string(let_type), vt_to_string(expr_type));
    }

    return node;
}

/**
 * @fn CTAbinop
 */
node_st *CTAbinop(node_st *node)
{
    TRAVleft(node);
    const ValueType left_type = last_type;
    const bool left_is_arith = IS_ARITH_TYPE(left_type);

    TRAVright(node);
    const ValueType right_type = last_type;
    const bool right_is_arith = IS_ARITH_TYPE(right_type);

    // Numbers are compatible
    if (left_is_arith && right_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        last_type = left_type == VT_FLOAT || right_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (left_type == VT_BOOL && right_type == VT_BOOL) {
        last_type = VT_BOOL;
    }

    else {
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_string(left_type), vt_to_string(right_type));
    }

    return node;
}

/**
 * @fn CTAmonop
 */
node_st *CTAmonop(node_st *node)
{
    TRAVexpr(node);

    // Confirm type is correct
    switch (MONOP_OP(node)) {
        // TODO: Find out if negated bool switches values (probably not)
        case MO_neg: if (!(last_type == VT_NUM || last_type == VT_FLOAT)) {
            USER_ERROR("Expected operand type Int or Float, got %s instead", vt_to_string(last_type));
        }
        break;
        case MO_not: if (last_type != VT_BOOL) {
            USER_ERROR("Expected operand type Bool, got %s instead", vt_to_string(last_type));
        }
        break;
        default: /* Should never occur */
            USER_ERROR("Unexpected error comparing monop type and expected type");
    }

    return node;
}

/**
 * @fn CTAvarlet
 */
node_st *CTAvarlet(node_st *node)
{
    // TODO: Add array support

    char* name = VAR_NAME(node);

    // Look up variable
    Symbol* s = STSlookup(STS, VARLET_NAME(node));

    HANDLE_MISSING_SYMBOL(name, s);

    last_type = s->vtype;

    // Create new entry on indexing stack
    DLSpush(DLS);

    // TODO: activate and deactivate saving to DLS based on global parameter
    // SAVE_DIMS = true;
    TRAVdims(node);
    // SAVE_DIMS = false;

    // Save information to array symbol
    DimsList* dml = DLSpeekTop(DLS);
    s->as.array.dim_count = dml->size;
    s->as.array.dims = dml->dims;

    // Clean up, except for dims list
    DMLfree(&dml);

    return node;
}

/**
 * @fn CTAvar
 */
node_st *CTAvar(node_st *node)
{
    // TODO: Add array support

    char* name = VAR_NAME(node);

    // Look up variable
    Symbol* s = STSlookup(STS, name);

    HANDLE_MISSING_SYMBOL(name, s);

    last_type = s->vtype;

    // Create new entry on indexing stack
    DLSpush(DLS);

    // TODO: activate and deactivate saving to DLS based on global parameter
    // SAVE_DIMS = true;
    TRAVdims(node);
    // SAVE_DIMS = false;

    // Save information to array symbol
    DimsList* dml = DLSpeekTop(DLS);
    s->as.array.dim_count = dml->size;
    s->as.array.dims = dml->dims;

    // Clean up, except for dims list
    DMLfree(&dml);

    return node;
}

/**
 * @fn CTAnum
 */
node_st *CTAnum(node_st *node)
{
    TRAVchildren(node);
    last_type = VT_NUM;
    return node;
}

/**
 * @fn CTAfloat
 */
node_st *CTAfloat(node_st *node)
{
    TRAVchildren(node);
    last_type = VT_FLOAT;
    return node;
}

/**
 * @fn CTAbool
 */
node_st *CTAbool(node_st *node)
{
    TRAVchildren(node);
    last_type = VT_BOOL;
    return node;
}

