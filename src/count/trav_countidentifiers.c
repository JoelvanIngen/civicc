/**
 * @file
 *
 * Traversal: CountIdentifiers
 * UID      : CI
 *
 *
 */

#include <stdio.h>

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"
#include "palm/memory.h"

// Prototypes
void freeTableValues(htable_st *t);
// /Prototypes

void CIinit() {
    DATA_CI_GET()->identifier_counts = HTnew_String(10);
}

void CIfini() {
    htable_st *t = DATA_CI_GET()->identifier_counts;
    freeTableValues(t);
    HTdelete(t);
}

/**
 * Frees all htable values to prevent memory leaks
 */
void freeTableValues(htable_st *t) {
    for (htable_iter_st *iter = HTiterate(t);
            iter; iter = HTiterateNext(iter)) {
        MEMfree(HTiterValue(iter));
    }
}

void printSingleIdentifier(htable_iter_st *iter) {
    char *key = (char *) HTiterKey(iter);
    int *value = (int *) HTiterValue(iter);

    printf("%s:\t\t%i\n", key, *value);
}

/**
 * Iterates over the htable and calls the printer helper function
 */
void printIdentifierCounts() {
    htable_st *t = DATA_CI_GET()->identifier_counts;
    for (htable_iter_st *iter = HTiterate(t);
            iter; iter = HTiterateNext(iter)) {
        printSingleIdentifier(iter);
    }
}

/**
 * Helper function that looks up a table from a key,
 * and increments it. Inserts a one if the value did
 * not exist yet.
 */
void tableIncrementOrCreate(htable_st *t, void *key) {
    void *v = HTlookup(t, key);
    if (v == NULL) {
        int *new_val = malloc(sizeof(int));
        *new_val = 1;
        HTinsert(t, key, new_val);
    } else {
        int *val = (int *) v;
        (*val)++;
    }
}

void tableIncrement(void *key) {
    htable_st *t = DATA_CI_GET()->identifier_counts;
    tableIncrementOrCreate(t, key);
}

/**
 * Increments or creates the appropriate var counter
 * in the hashtable for a Var node
 */
void countVar(node_st *node) {
    void *key = VAR_NAME(node);
    tableIncrement(key);
}

/**
 * Increments or creates the appropriate var counter
 * in the hashtable for a VarLet node
 */
void countVarLet(node_st *node) {
    void *key = VARLET_NAME(node);
    tableIncrement(key);
}

/**
 * @fn CIprogram
 */
node_st *CIprogram(node_st *node)
{
    TRAVchildren(node);
    printIdentifierCounts();
    return node;
}

/**
 * @fn CIvar
 */
node_st *CIvar(node_st *node)
{
    TRAVchildren(node);
    countVar(node);
    return node;
}

/**
 * @fn CIvarlet
 */
node_st *CIvarlet(node_st *node)
{
    TRAVchildren(node);
    countVarLet(node);
    return node;
}

