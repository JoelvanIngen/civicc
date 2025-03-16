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

FILE* ASM_FILE;
Assembly ASM;

static void init() {
    // TODO: probably make filename command-line argument
    ASM_FILE = fopen("bytecode.asm", "w");
    if (ASM_FILE == NULL) {
        fprintf(stderr, "Error creating bytecode file");
        exit(1);
    }
}

static void fini() {
    write_assembly(ASM_FILE, &ASM);
    fclose(ASM_FILE);
}

/**
 * @fn BCprogram
 */
node_st *BCprogram(node_st *node)
{
    init();

    TRAVchildren(node);

    fini();
    return node;
}

/**
 * @fn BCdecls
 */
node_st *BCdecls(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCexprs
 */
node_st *BCexprs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCarrexpr
 */
node_st *BCarrexpr(node_st *node)
{
    TRAVchildren(node);
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
    return node;
}

/**
 * @fn BCreturn
 */
node_st *BCreturn(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfuncall
 */
node_st *BCfuncall(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCcast
 */
node_st *BCcast(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfundefs
 */
node_st *BCfundefs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfundef
 */
node_st *BCfundef(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfunbody
 */
node_st *BCfunbody(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCifelse
 */
node_st *BCifelse(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCwhile
 */
node_st *BCwhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCdowhile
 */
node_st *BCdowhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfor
 */
node_st *BCfor(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCglobdecl
 */
node_st *BCglobdecl(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCglobdef
 */
node_st *BCglobdef(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCparam
 */
node_st *BCparam(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCvardecl
 */
node_st *BCvardecl(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCstmts
 */
node_st *BCstmts(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCassign
 */
node_st *BCassign(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCbinop
 */
node_st *BCbinop(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCmonop
 */
node_st *BCmonop(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCvarlet
 */
node_st *BCvarlet(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCvar
 */
node_st *BCvar(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCnum
 */
node_st *BCnum(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCfloat
 */
node_st *BCfloat(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn BCbool
 */
node_st *BCbool(node_st *node)
{
    TRAVchildren(node);
    return node;
}

