/**
 * @file
 *
 * Traversal: Eval
 * UID      : EV
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

/**
 * @fn EVnum
 */
node_st *EVnum(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn EVfloat
 */
node_st *EVfloat(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn EVbool
 */
node_st *EVbool(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn EVbinop
 */
node_st *EVbinop(node_st *node)
{
    TRAVchildren(node);

    node_st *new = NULL;
    switch(BINOP_OP(node)) {
        case BO_add: new = ASTnum(NUM_VAL(BINOP_LEFT(node)) + NUM_VAL(BINOP_RIGHT(node))); break;
        default: break;
    }
    return new;
}

/**
 * @fn EVvar
 */
node_st *EVvar(node_st *node)
{
    TRAVchildren(node);
    return node;
}

