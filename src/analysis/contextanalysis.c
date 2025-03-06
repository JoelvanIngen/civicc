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
#include "variables/paramstack.h"
#include "variables/variable.h"

// Shortcuts
typedef ScopeValue SV;

#define IS_ARITH_TYPE(vt) (vt == SV_VT_NUM || vt == SV_VT_FLOAT)

typedef enum {
    DECLARATION_PASS,
    ANALYSIS_PASS,
} PassType;

PassType PASS;
VType last_type;

VarTableStack* VTS;
ParamListStack* PLS;
SV* param_parentfun_ptr = NULL;

void CTAinit() {
    VTS = VTSnew();
    PLS = PLSnew();
}
void CTAfini() {
    // VTSdestroy also deletes any leftovers, we don't worry about cleaning up
    VTSdestroy(VTS);
    PLSfree(&PLS);
}

VType vtype_from_nt_type(const enum Type ct_type, const bool is_array) {
    switch (ct_type) {
        case CT_int: return is_array ? SV_VT_NUM_ARRAY : SV_VT_NUM;
        case CT_float: return is_array ? SV_VT_FLOAT_ARRAY : SV_VT_FLOAT;
        case CT_bool: return is_array ? SV_VT_BOOL_ARRAY : SV_VT_BOOL;
        case CT_void:
            if (is_array) {
                // Doesn't exist - TODO: find out if actually doesn't exist
                printf("Type error: cannot have a void array\n");
                exit(1);
            }
            return SV_VT_VOID;
        default:
            printf("Type error: unexpected ct_type %i\n", ct_type);
        exit(1);
    }
}

/**
 * @fn CTAprogram
 */
node_st *CTAprogram(node_st *node)
{
    // Create global scope
    VTSpush(VTS);

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
    return node;
}

/**
 * @fn CTAfuncall
 */
node_st *CTAfuncall(node_st *node)
{
    // Check if function exists and is actually a function
    SV* sv = VTSfind(VTS, FUNCALL_NAME(node));
    if (!sv) {
        USER_ERROR("Function name %s does not exist in scope.", FUNCALL_NAME(node));
        last_type = SV_VT_VOID;
        return node;
    }

    if (sv->node_t != SV_NT_FN) {
        USER_ERROR("Name %s is not defined as a function.", FUNCALL_NAME(node));
        last_type = SV_VT_VOID;
        return node;
    }

    PLSpush(PLS);
    TRAVchildren(node);

    const Parameter* args = PLSgetCurrentArgs(PLS);
    const size_t args_len = PLSgetCurrentLength(PLS);

    const VType* param_ts = sv->as.array.data;
    const size_t params_len = sv->as.array.dim_count;

    if (args_len > params_len) {
        USER_ERROR("Too many arguments; got %lu but function %s only expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
    }

    if (args_len < params_len) {
        USER_ERROR("Not enough arguments; got %lu but function %s expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
    }

    for (int i = 0; i < params_len; i++) {
        if (args->type != param_ts[i]) {
            // TODO: Make more descriptive
            USER_ERROR("Argument type and parameter type don't match");
        }
    }

    PLSpop(PLS);

    last_type = GET_TYPE(sv);
    return node;
}

/**
 * @fn CTAcast
 */
node_st *CTAcast(node_st *node)
{
    TRAVchildren(node);
    if (!(last_type == SV_VT_NUM || last_type == SV_VT_FLOAT || last_type == SV_VT_BOOL)) {
        // TODO: Create enum-to-str function to better print errors instead of enum values
        USER_ERROR("Invalid cast source, can only cast from Num, Float or Bool but got %i", last_type);
    }

    last_type = CAST_TYPE(node);
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
    if (PASS == DECLARATION_PASS) {
        /* First pass: only return information about self */

        const VType type = vtype_from_nt_type(FUNDEF_TYPE(node), false);
        SV* sv = SVfromFun(type);
        VTSadd(VTS, FUNDEF_NAME(node), sv);
    } else {
        /* Second pass: explore information about own statements
         * Here we also detect if variables are wrongly typed */

        // Add local scope to stack
        VTSpush(VTS);

        // Add parameters to scope
#ifdef DEBUGGING
        ASSERT_MSG((param_parentfun_ptr != NULL), "Starting to add arguments, but apparently was already doing so");
#endif // DEBUGGING
        // Find this function that we added to the scope one level down
        param_parentfun_ptr = VTSfind(VTS, FUNDEF_NAME(node));
        TRAVparams(node);
        param_parentfun_ptr = NULL;

        // Explore function body
        TRAVbody(node);

        // Discard scope as it is no longer needed
        VTSpop(VTS);
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
    // TODO: Find out if multiple similarly named variables overwrite the old one
    // TODO continuation: Otherwise display error here instead
    SV* sv = SVfromVar(SV_VT_NUM, NULL);
    VTSadd(VTS, FOR_VAR(node), sv);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAglobdecl
 */
node_st *CTAglobdecl(node_st *node)
{
    // Add self to vartable of current scope
    // TODO: Find out if multiple similarly named variables overwrite the old one
    // TODO continuation: Otherwise display error here instead
    // TODO: Add array support
    SV* sv = SVfromVar(GLOBDEF_TYPE(node), NULL);
    VTSadd(VTS, GLOBDEF_NAME(node), sv);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAglobdef
 */
node_st *CTAglobdef(node_st *node)
{
    // Add self to vartable of current scope
    // TODO: Find out if multiple similarly named variables overwrite the old one
    // TODO continuation: Otherwise display error here instead
    // TODO: Add array support
    SV* sv = SVfromVar(GLOBDEF_TYPE(node), NULL);
    VTSadd(VTS, GLOBDEF_NAME(node), sv);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAparam
 */
node_st *CTAparam(node_st *node)
{
    // Add self to vartable of current scope
    // TODO: Find out if multiple similarly named variables overwrite the old one
    // TODO continuation: Otherwise display error here instead
    // TODO: Add array support

    char* name = PARAM_NAME(node);
    const VType type = vtype_from_nt_type(PARAM_TYPE(node), false);

    // Add parameter as variable for next scope
    SV* sv = SVfromVar(type, NULL);
    VTSadd(VTS, name, sv);

    // Add parameter to parameter list expected for funcalls
    PLadd(param_parentfun_ptr->as.function, name, type);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAvardecl
 */
node_st *CTAvardecl(node_st *node)
{
    // Add self to vartable of current scope
    // TODO: Find out if multiple similarly named variables overwrite the old one
    // TODO continuation: Otherwise display error here instead
    // TODO: Add array support
    SV* sv = SVfromVar(VARDECL_TYPE(node), NULL);
    VTSadd(VTS, VARDECL_NAME(node), sv);

    TRAVchildren(node);
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
    const VType let_type = last_type;
    const bool let_is_arith = IS_ARITH_TYPE(let_type);

    TRAVchildren(node);
    const VType expr_type = last_type;
    const bool expr_is_arith = IS_ARITH_TYPE(expr_type);

    // Numbers are compatible, although implicit casting will take place
    if (let_is_arith && expr_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        last_type = let_type == SV_VT_FLOAT || expr_type == SV_VT_FLOAT ? SV_VT_FLOAT : SV_VT_NUM;
    }

    // Booleans are compatible
    else if (let_type == SV_VT_BOOL && expr_type == SV_VT_BOOL) {
        last_type = SV_VT_BOOL;
    }

    else {
        // TODO: Create enum-to-str function to better print errors instead of enum values
        USER_ERROR("Operands not compatible: %i and %i", let_type, expr_type);
    }

    return node;
}

/**
 * @fn CTAbinop
 */
node_st *CTAbinop(node_st *node)
{
    TRAVleft(node);
    const VType left_type = last_type;
    const bool left_is_arith = IS_ARITH_TYPE(left_type);

    TRAVright(node);
    const VType right_type = last_type;
    const bool right_is_arith = IS_ARITH_TYPE(right_type);

    // Numbers are compatible
    if (left_is_arith && right_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        last_type = left_type == SV_VT_FLOAT || right_type == SV_VT_FLOAT ? SV_VT_FLOAT : SV_VT_NUM;
    }

    // Booleans are compatible
    else if (left_type == SV_VT_BOOL && right_type == SV_VT_BOOL) {
        last_type = SV_VT_BOOL;
    }

    else {
        // TODO: Create enum-to-str function to better print errors instead of enum values
        USER_ERROR("Operands not compatible: %i and %i", left_type, right_type);
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
        case MO_neg: if (!(last_type == SV_VT_NUM || last_type == SV_VT_FLOAT)) {
            // TODO: Create enum-to-str function to better print errors instead of enum values
            USER_ERROR("Expected operand type Num or Float, got %i instead", last_type);
        }
        break;
        case MO_not: if (last_type != SV_VT_BOOL) {
            // TODO: Create enum-to-str function to better print errors instead of enum values
            USER_ERROR("Expected operand type Bool, got %i instead", last_type);
        }
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

    TRAVchildren(node);

    // Look up variable
    SV* sv = VTSfind(VTS, VAR_NAME(node));
    if (!sv) {
        // TODO: Show error that variable doesn't exist
        // Exit for now to prevent IDE warnings
        exit(1);
    }

    last_type = sv->data_t;
    return node;
}

/**
 * @fn CTAvar
 */
node_st *CTAvar(node_st *node)
{
    // TODO: Add array support

    TRAVchildren(node);

    // Look up variable
    SV* sv = VTSfind(VTS, VAR_NAME(node));
    if (!sv) {
        // TODO: Show error that variable doesn't exist
        // Exit for now to prevent IDE warnings
        exit(1);
    }

    last_type = sv->data_t;
    return node;
}

/**
 * @fn CTAnum
 */
node_st *CTAnum(node_st *node)
{
    TRAVchildren(node);
    last_type = SV_VT_NUM;
    return node;
}

/**
 * @fn CTAfloat
 */
node_st *CTAfloat(node_st *node)
{
    TRAVchildren(node);
    last_type = SV_VT_FLOAT;
    return node;
}

/**
 * @fn CTAbool
 */
node_st *CTAbool(node_st *node)
{
    TRAVchildren(node);
    last_type = SV_VT_BOOL;
    return node;
}

