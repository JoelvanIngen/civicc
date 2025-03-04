/**
 * @file
 *
 * Traversal: SyntacticAnalysis
 * UID      : SAN
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

#include "common.h"
#include "variables/variable.h"

// Shortcuts
#define VTS DATA_SAN_GET()->vartable_stack
#define SV ScopeValue

typedef enum {
    DECLARATION_PASS,
    ANALYSIS_PASS,
} PassType;

PassType pass;

void SANinit() {
    VTS = VTSnew();
}
void SANfini() {
    // VTSdestroy also deletes any leftovers, we don't worry about cleaning up
    VTSdestroy(VTS);
}

VType vtype_from_nt_type(const enum Type ct_type, const bool is_array) {
    switch (ct_type) {
        case CT_int: return is_array ? NUM_ARRAY : NUM;
        case CT_float: return is_array ? FLOAT_ARRAY : FLOAT;
        case CT_bool: return is_array ? BOOL_ARRAY : BOOL;
        case CT_void:
            if (is_array) {
                // Doesn't exist - TODO: find out if actually doesn't exist
                printf("Type error: cannot have a void array\n");
                exit(1);
            }

            return VOID;
        default:
            printf("Type error: unexpected ct_type %i\n", ct_type);
            exit(1);
    }
}

/**
 * @fn SANprogram
 */
node_st *SANprogram(node_st *node)
{
    // Create global scope
    VTSpush(VTS);

    /* First pass; we only look at function definitions, so we can correctly
     * handle usages of functions that have not been defined yet
     */
    pass = DECLARATION_PASS;
    TRAVchildren(node);

    /* Second pass; we perform full analysis of the body now that we know
     * all definitions
     */
    pass = ANALYSIS_PASS;
    TRAVchildren(node);

    return node;
}

/**
 * @fn SANdecls
 */
node_st *SANdecls(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANexprs
 */
node_st *SANexprs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANarrexpr
 */
node_st *SANarrexpr(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANids
 */
node_st *SANids(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANexprstmt
 */
node_st *SANexprstmt(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANreturn
 */
node_st *SANreturn(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANfuncall
 */
node_st *SANfuncall(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANcast
 */
node_st *SANcast(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANfundefs
 */
node_st *SANfundefs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANfundef
 */
node_st *SANfundef(node_st *node)
{
    if (pass == DECLARATION_PASS) {
        const VType type = vtype_from_nt_type(FUNDEF_TYPE(node), false);
        SV* sv = SVfromFun(type);
        VTSadd(VTS, FUNDEF_NAME(node), sv);
    } else {
        // Add local scope to stack
        VTSpush(VTS);

        // Add parameters to scope
        TRAVparams(node);

        // Explore inner functions and declarations that might be used by inner statements
        pass = DECLARATION_PASS;
        TRAVdecls(node);
        TRAVlocal_fundefs(node);

        // Explore local functions and own statements now that we gathered all declarations
        pass = ANALYSIS_PASS;
        TRAVlocal_fundefs(node);
        TRAVstmts(node);

        // Discard scope as it is no longer needed
        VTSpop(VTS);
    }

    return node;
}

/**
 * @fn SANfunbody
 */
node_st *SANfunbody(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANifelse
 */
node_st *SANifelse(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANwhile
 */
node_st *SANwhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANdowhile
 */
node_st *SANdowhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANfor
 */
node_st *SANfor(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANglobdecl
 */
node_st *SANglobdecl(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANglobdef
 */
node_st *SANglobdef(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANparam
 */
node_st *SANparam(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANvardecl
 */
node_st *SANvardecl(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANstmts
 */
node_st *SANstmts(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANassign
 */
node_st *SANassign(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANbinop
 */
node_st *SANbinop(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANmonop
 */
node_st *SANmonop(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANvarlet
 */
node_st *SANvarlet(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANvar
 */
node_st *SANvar(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANnum
 */
node_st *SANnum(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANfloat
 */
node_st *SANfloat(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn SANbool
 */
node_st *SANbool(node_st *node)
{
    TRAVchildren(node);
    return node;
}

