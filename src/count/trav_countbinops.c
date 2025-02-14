/**
 * @file
 *
 * Traversal: CountBinops
 * UID      : CBO
 *
 *
 */

#include <stdio.h>

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

void CBOinit() { return; }
void CBOfini() { return; }

/**
 * Helper function that copies the travdata
 * to program attributes
 */
void saveCountsToProgram(node_st *node) {
    PROGRAM_SUM_ADD(node) = DATA_CBO_GET()->sum_add;
    PROGRAM_SUM_SUB(node) = DATA_CBO_GET()->sum_sub;
    PROGRAM_SUM_MUL(node) = DATA_CBO_GET()->sum_mul;
    PROGRAM_SUM_DIV(node) = DATA_CBO_GET()->sum_div;
    PROGRAM_SUM_MOD(node) = DATA_CBO_GET()->sum_mod;
}

/**
 * @fn CBObinop
 */
node_st *CBObinop(node_st *node)
{
    TRAVchildren(node);

    struct data_cbo *data = DATA_CBO_GET();
    switch (BINOP_OP(node)) {
        case BO_add: data->sum_add++; break;
        case BO_sub: data->sum_sub++; break;
        case BO_mul: data->sum_mul++; break;
        case BO_div: data->sum_div++; break;
        case BO_mod: data->sum_mod++; break;
        default: break;
    }
    return node;
}

/**
 * @fn CBOprogram
 */
node_st *CBOprogram(node_st *node)
{
    TRAVchildren(node);
    saveCountsToProgram(node);
    return node;
}

