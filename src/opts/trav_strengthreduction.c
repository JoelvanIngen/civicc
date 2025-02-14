/**
 * @file
 *
 * Traversal: StrengthReduction
 * UID      : OSR
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

// #include "global/globals.h"

static bool areChildTypesEligible(node_st *node, node_st **found_var, node_st **found_int) {
    if (NODE_TYPE(BINOP_LEFT(node)) == NT_VAR && NODE_TYPE(BINOP_RIGHT(node)) == NT_NUM) {
        *found_var = BINOP_LEFT(node);
        *found_int = BINOP_RIGHT(node);
    } else if (NODE_TYPE(BINOP_LEFT(node)) == NT_NUM && NODE_TYPE(BINOP_RIGHT(node)) == NT_VAR) {
        *found_int = BINOP_LEFT(node);
        *found_var = BINOP_RIGHT(node);
    } else { return false; }

    return true;
}

static bool isIntEligible(int num) {
    // TODO: Maximum int_node value to optimise (cmd-line option)
    // Until I implement the cmd option I'll just use a magic number
    int max_opt = 10;

    // Don't optimise past max values
    if (num < -max_opt || num > max_opt) return false;

    // Can't optimise n = 1 or 0, this is for different opts
    if (num > -2 && num < 2) return false;

    return true;
}

/**
 * @fn OSRbinop
 */
node_st *OSRbinop(node_st *node)
{
    TRAVchildren(node);

    // We only target multiplication; early exit if not mul operator
    if (BINOP_OP(node) != BO_mul) return node;

    // We need one variable and one integer
    node_st *var_node;
    node_st *int_node;
    if (!areChildTypesEligible(node, &var_node, &int_node)) return node;

    int num = NUM_VAL(int_node);
    if (!isIntEligible(num)) return node;
    
    node_st *new_node = ASTbinop(CCNcopy(var_node), CCNcopy(var_node), BO_add);
    for (int i = 2; i < num; i++) {
        new_node = ASTbinop(new_node, CCNcopy(var_node), BO_add);
    }

    CCNfree(node);

    return new_node;
}
