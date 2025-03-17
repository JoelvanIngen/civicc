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

    /**
     * Emit function label
     * Emit "esr N" with N being the amount of variables we are going to use in function
     * Traverse children
     * !!! TODO: figure out a way to find the amount of variables used in function
     * !!! TODO: figure out if for-loop variables count for that purpose (would make everything a tad trickier)
     */
    return node;
}

/**
 * @fn BCifelse
 */
node_st *BCifelse(node_st *node)
{
    TRAVchildren(node);

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
    TRAVchildren(node);

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
 * @fn BCdowhile
 */
node_st *BCdowhile(node_st *node)
{
    TRAVchildren(node);

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
 * @fn BCfor
 */
node_st *BCfor(node_st *node)
{
    TRAVchildren(node);

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
    TRAVchildren(node);

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
    TRAVchildren(node);

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
    TRAVchildren(node);

    /**
     * Match whether it has an expression:
     * --- No: Probably return since it's only here for the compiler to know the type
     * --- Yes: Treat as an assign; push value to stack
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
    TRAVchildren(node);

    /**
     * Do nothing? Probably already handled by varlet node, which pops the upper value from stack
     */
    return node;
}

/**
 * @fn BCbinop
 */
node_st *BCbinop(node_st *node)
{
    TRAVchildren(node);

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
    TRAVchildren(node);

    /**
     * Find which scope variable is from
     * Match:
     * --- Own scope: find index of variable in scope + save using "Store Local Variable" (1.4.5)
     * --- Higher frame: find index of variable in scope + save using "Store Relatively Free Variable" (1.4.5)
     * --- Global: find index of variable in scope + save using "Store Global Variable" (1.4.5)
     * !!! Storing to imported variables is impossible but already stopped in contextanalysis
     * !!! Needs array handling
     */
    return node;
}

/**
 * @fn BCvar
 */
node_st *BCvar(node_st *node)
{
    TRAVchildren(node);

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
    return node;
}

