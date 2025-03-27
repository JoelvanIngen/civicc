/**
 * @file
 *
 * This file contains the code for the Print traversal.
 * The traversal has the uid: PRT
 *
 *
 */

#include "common.h"
#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav.h"
#include "palm/dbug.h"

static int INDENT = 0;

static char *TRUE_STRING = "true";
static char *FALSE_STRING = "false";

static char *BOOL_STRING = "bool";
static char *INT_STRING = "int";
static char *FLOAT_STRING = "float";
static char *VOID_STRING = "void";

static char *MONOP_OP_STRINGS[] = {"", "!", "-"};
static char *BINOP_OP_STRINGS[] = {"", "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "||"};

static char *bool_to_string(const bool b) {
    return b ? TRUE_STRING : FALSE_STRING;
}

static char *type_to_string(const enum Type type) {
    switch(type) {
        case CT_bool: return BOOL_STRING;
        case CT_int: return INT_STRING;
        case CT_float: return FLOAT_STRING;
        case CT_void: return VOID_STRING;
        default: fprintf(stderr, "UNKNOWN TYPE"); return NULL;
    }
}

static void print_indent() {
    for(int i = 0; i < INDENT; i++) {
        printf("\t");
    }
}



/**
 * @fn PRTprogram
 */
node_st *PRTprogram(node_st *node)
{
    printf("START OF PROGRAM");
    TRAVchildren(node);
    printf("\nEND OF PROGRAM\n");
    return node;
}

/**
 * @fn PRTdecls
 */
node_st *PRTdecls(node_st *node)
{
    if (DECLS_DECL(node) != NULL) {
        printf("\n");
        TRAVdecl(node);
    }
    TRAVnext(node);
    return node;
}

/**
 * @fn PRTexprs
 */
node_st *PRTexprs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTarrexpr
 */
node_st *PRTarrexpr(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTids
 */
node_st *PRTids(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTexprstmt
 */
node_st *PRTexprstmt(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTreturn
 */
node_st *PRTreturn(node_st *node)
{
    printf("RETURN(");
    TRAVchildren(node);
    printf(")");
    return node;
}

/**
 * @fn PRTfuncall
 */
node_st *PRTfuncall(node_st *node)
{
    printf("FUNCALL(name=%s", FUNCALL_NAME(node));
    TRAVchildren(node);
    printf(")");
    return node;
}

/**
 * @fn PRTcast
 */
node_st *PRTcast(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfundefs
 */
node_st *PRTfundefs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfundef
 */
node_st *PRTfundef(node_st *node)
{
    const bool has_body = FUNDEF_BODY(node) != NULL;

    print_indent();

    if (has_body) printf("BEGIN ");

    printf("FUNDEF(name=%s, type=%s",
        FUNDEF_NAME(node), type_to_string(FUNDEF_TYPE(node)));
    
    // Print args if they exist
    if (FUNDEF_PARAMS(node) != NULL) {
        printf(", params=(");
        TRAVparams(node);
        printf(")");
    }

    printf(")");
    
    if (has_body) {
        INDENT++;
        TRAVchildren(node);
        INDENT--;
        printf("\n");
        print_indent();
        printf("END FUNDEF(name=%s)", FUNDEF_NAME(node));
    }
    
    
    return node;
}

/**
 * @fn PRTfunbody
 */
node_st *PRTfunbody(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTifelse
 */
node_st *PRTifelse(node_st *node)
{
    printf("START IF(cond=");
    TRAVcond(node);
    printf(")");
    INDENT++;
    TRAVthen(node);
    INDENT--;
    printf("\n");
    if IFELSE_ELSE_BLOCK(node) {
        print_indent();
        printf("ELSE");
        INDENT++;
        TRAVelse_block(node);
        INDENT--;
        printf("\n");
    }
    print_indent();
    printf("END IF");
    return node;
}

/**
 * @fn PRTwhile
 */
node_st *PRTwhile(node_st *node)
{
    printf("START WHILE(cond=");
    TRAVcond(node);
    printf(")");
    INDENT++;
    TRAVblock(node);
    INDENT--;
    printf("\n");
    print_indent();
    printf("END WHILE");
    return node;
}

/**
 * @fn PRTdowhile
 */
node_st *PRTdowhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfor
 */
node_st *PRTfor(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTglobdecl
 */
node_st *PRTglobdecl(node_st *node)  // TODO: ADD DIMS
{
    printf("GLOBDECL(name=%s, type=%s)",
        GLOBDECL_NAME(node), type_to_string(GLOBDECL_TYPE(node)));
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTglobdef
 */
node_st *PRTglobdef(node_st *node)  // TODO: ADD DIMS
{
    printf("GLOBDEF(export=%s name=%s, type=%s)",
        bool_to_string(GLOBDEF_EXPORT(node)), GLOBDEF_NAME(node), type_to_string(GLOBDEF_TYPE(node)));

    if (GLOBDEF_INIT(node) != NULL) {
        printf(" <- ");
        TRAVinit(node);
    }

    return node;
}

/**
 * @fn PRTparam
 */
node_st *PRTparam(node_st *node)
{
    printf("%s %s", ct_to_str(PARAM_TYPE(node)), PARAM_NAME(node));
    if (PARAM_NEXT(node) != NULL) {
        printf(", ");
    }
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTvardecl
 */
node_st *PRTvardecl(node_st *node)
{
    printf("\n");
    print_indent();
    printf("VARDECL(name=%s, type=%s)",
        VARDECL_NAME(node), type_to_string(VARDECL_TYPE(node)));

    if VARDECL_INIT(node) {
        printf(" <- ");
    }
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTstmts
 */
node_st *PRTstmts(node_st *node)
{
    printf("\n");
    print_indent();
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTassign
 */
node_st *PRTassign(node_st *node)
{
    printf("ASSIGN(");
    TRAVlet(node);
    printf(" <- ");
    TRAVexpr(node);
    printf(")");
    return node;
}

/**
 * @fn PRTbinop
 */
node_st *PRTbinop(node_st *node)
{
    printf("BINOP(");
    TRAVleft(node);
    printf(" %s ", BINOP_OP_STRINGS[BINOP_OP(node)]);
    TRAVright(node);
    printf(")");
    return node;
}

/**
 * @fn PRTmonop
 */
node_st *PRTmonop(node_st *node)
{
    printf("MONOP(%s", MONOP_OP_STRINGS[MONOP_OP(node)]);
    TRAVchildren(node);
    printf(")");
    return node;
}

/**
 * @fn PRTvarlet
 */
node_st *PRTvarlet(node_st *node)
{
    printf("VARLET(%s)", VARLET_NAME(node));
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTvar
 */
node_st *PRTvar(node_st *node)
{
    printf("VAR(%s)", VAR_NAME(node));
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTnum
 */
node_st *PRTnum(node_st *node)
{
    printf("NUM(%i)", NUM_VAL(node));
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfloat
 */
node_st *PRTfloat(node_st *node)
{
    printf("FLOAT(%f)", FLOAT_VAL(node));
    TRAVchildren(node);
    return node;
}

/**
 * @fn PRTbool
 */
node_st *PRTbool(node_st *node)
{
    printf("BOOL(%s)", bool_to_string(BOOL_VAL(node)));
    TRAVchildren(node);
    return node;
}

//
// /**
//  * @fn PRTassign
//  */
// node_st *PRTassign(node_st *node)
// {
//
//     if (ASSIGN_LET(node) != NULL) {
//         TRAVlet(node);
//         printf( " = ");
//     }
//
//     TRAVexpr(node);
//     printf( ";\n");
//
//
//     return node;
// }
//
// /**
//  * @fn PRTbinop
//  */
// node_st *PRTbinop(node_st *node)
// {
//     char *tmp = NULL;
//     printf( "( ");
//
//     TRAVleft(node);
//
//     switch (BINOP_OP(node)) {
//     case BO_add:
//       tmp = "+";
//       break;
//     case BO_sub:
//       tmp = "-";
//       break;
//     case BO_mul:
//       tmp = "*";
//       break;
//     case BO_div:
//       tmp = "/";
//       break;
//     case BO_mod:
//       tmp = "%";
//       break;
//     case BO_lt:
//       tmp = "<";
//       break;
//     case BO_le:
//       tmp = "<=";
//       break;
//     case BO_gt:
//       tmp = ">";
//       break;
//     case BO_ge:
//       tmp = ">=";
//       break;
//     case BO_eq:
//       tmp = "==";
//       break;
//     case BO_ne:
//       tmp = "!=";
//       break;
//     case BO_or:
//       tmp = "||";
//       break;
//     case BO_and:
//       tmp = "&&";
//       break;
//     case BO_NULL:
//       DBUG_ASSERT(false, "unknown binop detected!");
//     }
//
//     printf( " %s ", tmp);
//
//     TRAVright(node);
//
//     printf( ")(%d:%d-%d)", NODE_BLINE(node), NODE_BCOL(node), NODE_ECOL(node));
//
//     return node;
// }
//
// /**
//  * @fn PRTvarlet
//  */
// node_st *PRTvarlet(node_st *node)
// {
//     printf("%s(%d:%d)", VARLET_NAME(node), NODE_BLINE(node), NODE_BCOL(node));
//     return node;
// }
//
// /**
//  * @fn PRTvar
//  */
// node_st *PRTvar(node_st *node)
// {
//     printf( "%s", VAR_NAME(node));
//     return node;
// }
//
// /**
//  * @fn PRTnum
//  */
// node_st *PRTnum(node_st *node)
// {
//     printf("%d", NUM_VAL(node));
//     return node;
// }
//
// /**
//  * @fn PRTfloat
//  */
// node_st *PRTfloat(node_st *node)
// {
//     printf( "%f", FLOAT_VAL(node));
//     return node;
// }
//
// /**
//  * @fn PRTbool
//  */
// node_st *PRTbool(node_st *node)
// {
//     char *bool_str = BOOL_VAL(node) ? "true" : "false";
//     printf("%s", bool_str);
//     return node;
// }
