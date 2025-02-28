%{


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "palm/memory.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "palm/str.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "global/globals.h"

static node_st *parseresult = NULL;
extern int yylex();
int yyerror(char *errname);
extern FILE *yyin;
void AddLocToNode(node_st *node, void *begin_loc, void *end_loc);


%}

%union {
 char               *id;
 int                 cint;
 float               cflt;
 bool               cbool;
 enum MonOpType     cmonop;
 enum BinOpType     cbinop;
 enum Type          ctype;
 node_st             *node;
}

%locations

%token BRACE_L BRACE_R BRACKET_L BRACKET_R SBRACKET_L SBRACKET_R COMMA SEMICOLON
%token IF ELSE WHILE DO FOR RETURN
%token MINUS PLUS STAR SLASH PERCENT LE LT GE GT EQ NE OR AND
%token NOT
%token TRUEVAL FALSEVAL LET

%token <cint> NUM
%token <cflt> FLOAT
%token <id> ID
%token INTTYPE FLOATTYPE BOOLTYPE VOIDTYPE
%token EXPORT EXTERN

%type <node> program decls expr exprs arrexpr ids exprstmt
%type <node> returnstmt funcall cast fundefs fundef funbody
%type <node> ifstmt whilestmt dowhilestmt forstmt
%type <node> globdecl globdef param vardecl stmts
%type <node> assign varlet var
%type <node> intval floatval boolval
%type <cmonop> monop
%type <cbinop> binop
%type <ctype> type
%type <node> constants operations decl stmt

%type <node> opt_vardecls opt_fundefs opt_stmts opt_exprs
%type <node> opt_params param_list opt_funbody opt_ids opt_dims
%type <node> vardecls
%type <cbool> opt_export_bool

%type <node> primitive preclvl1 preclvl2 preclvl3 preclvl4 preclvl5 preclvl6

// Precedence rules
%left OR AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left STAR SLASH PERCENT
%right NOT
%right UMINUS

%start program

%%

program: decls
         {
           parseresult = ASTprogram($1);
         }
         ;

decls:  decl decls
        {
          $$ = ASTdecls($1, $2);
        }
      | decl
        {
          $$ = ASTdecls($1, NULL);
        }
        ;

decl: globdecl { $$ = $1; }
    | globdef { $$ = $1; }
    | fundef { $$ = $1; }
    ;

globdecl: EXTERN type[t] ids[dims] ID[name] SEMICOLON
          {
            $$ = ASTglobdecl($name, $t);
            GLOBDECL_DIMS($$) = $dims;
            AddLocToNode($$, &@t, &@name);
          }
        | EXTERN type[t] ID[name] SEMICOLON
          {
            $$ = ASTglobdecl($name, $t);
            AddLocToNode($$, &@t, &@name);
          }
          ;

globdef: opt_export_bool[export] type[t] opt_ids[dims] ID[name] LET expr[init] SEMICOLON
        {
          $$ = ASTglobdef($name, $t);
          GLOBDEF_DIMS($$) = $dims;
          GLOBDEF_INIT($$) = $init;
          GLOBDEF_EXPORT($$) = $export;
          AddLocToNode($$, &@t, &@init);
        }
        ;

opt_export_bool: EXPORT { $$ = true; } | /* Empty */ { $$ = false; };

opt_ids: ids { $$ = $1; } | /* Empty */ { $$ = NULL; };

stmts: stmt stmts
        {
          $$ = ASTstmts($1, $2);
        }
      | stmt
        {
          $$ = ASTstmts($1, NULL);
        }
        ;

stmt:   assign { $$ = $1; }
      | exprstmt { $$ = $1; }
      | ifstmt { $$ = $1; }
      | whilestmt { $$ = $1; }
      | dowhilestmt { $$ = $1; }
      | forstmt { $$ = $1; }
      | returnstmt { $$ = $1; }
      ;

opt_params: param_list { $$ = $1; } | /* Empty */ { $$ = NULL; };

param_list: param { $$ = $1; }
          | param_list COMMA param
            {
              $$ = $1;
              PARAM_NEXT($1) = $3;
            }
            ;

opt_funbody: funbody { $$ = $1; } | /* Empty */ { $$ = NULL; };

fundef: opt_export_bool[export] type[t] ID[name] BRACKET_L opt_params[params] BRACKET_R opt_funbody[body]
        {
          $$ = ASTfundef($name, $t);
          FUNDEF_PARAMS($$) = $params;
          FUNDEF_BODY($$) = $body;
          FUNDEF_EXPORT($$) = $export;
          AddLocToNode($$, &@t, &@name);
        }
        ;

fundefs: fundef fundefs
         {
          $$ = ASTfundefs($1);
          FUNDEFS_NEXT($$) = $2;
         }
       | fundef
         {
          $$ = ASTfundefs($1);
         }
         ;

opt_vardecls: vardecls { $$ = $1; } | /* Empty */ { $$ = NULL; };
opt_fundefs: fundefs { $$ = $1; } | /* Empty */ { $$ = NULL; };
opt_stmts: stmts { $$ = $1; } | /* Empty */ { $$ = NULL; };

funbody: BRACE_L[startbrace] opt_vardecls[decls] opt_fundefs[fundefs] opt_stmts[stmts] BRACE_R[endbrace]
          {
            $$ = ASTfunbody();
            FUNBODY_DECLS($$) = $decls;
            FUNBODY_LOCAL_FUNDEFS($$) = $fundefs;
            FUNBODY_STMTS($$) = $stmts;
            AddLocToNode($$, &@startbrace, &@endbrace);
          }
          ;

param: type[t] ID[id] ids[dims]
      {
        $$ = ASTparam($id, $t);
        PARAM_DIMS($$) = $dims;
        AddLocToNode($$, &@t, &@id);
      }
      ;

assign: varlet LET expr SEMICOLON
        {
          $$ = ASTassign($1, $3);
          AddLocToNode($$, &@1, &@3);
        }
        ;

exprstmt: expr SEMICOLON { $$ = ASTexprstmt($1); };

ifstmt: IF BRACKET_L expr[cond] BRACKET_R BRACE_L stmts[thenblock] BRACE_R ELSE BRACE_L stmts[elseblock] BRACE_R
        {
          $$ = ASTifelse($cond);
          IFELSE_THEN($$) = $thenblock;
          IFELSE_ELSE_BLOCK($$) = $elseblock;
          AddLocToNode($$, &@1, &@elseblock);
        }
      | IF BRACKET_L expr[cond] BRACKET_R BRACE_L stmts[thenblock] BRACE_R
        {
          $$ = ASTifelse($cond);
          IFELSE_THEN($$) = $thenblock;
          AddLocToNode($$, &@1, &@thenblock);
        }
        ;

whilestmt: WHILE BRACKET_L expr[cond] BRACKET_R BRACE_L stmts[block] BRACE_R
            {
              $$ = ASTwhile($cond);
              WHILE_BLOCK($$) = $block;
              AddLocToNode($$, &@1, &@block);
            }
          ;

dowhilestmt: DO BRACE_L stmts[block] BRACE_R WHILE BRACKET_L expr[cond] BRACKET_R
            {
              $$ = ASTdowhile($cond);
              DOWHILE_BLOCK($$) = $block;
            }
          ;

forstmt: FOR BRACKET_L ID[var] LET expr[init] COMMA expr[stop] COMMA expr[step] BRACKET_R BRACE_L stmts[block] BRACE_R
           {
             $$ = ASTfor($init, $stop);
             FOR_STEP($$) = $step;
             FOR_BLOCK($$) = $block;
             FOR_VAR($$) = $var;
           }
         | FOR BRACKET_L ID[var] LET expr[init] COMMA expr[stop] BRACKET_R BRACE_L stmts[block] BRACE_R
           {
             $$ = ASTfor($init, $stop);
             FOR_BLOCK($$) = $block;
             FOR_VAR($$) = $var;
           }
         ;

returnstmt: RETURN expr[val] SEMICOLON
            {
              $$ = ASTreturn();
              RETURN_EXPR($$) = $val;
            }
          | RETURN SEMICOLON
            {
              $$ = ASTreturn();
            }
          ;

funcall:  ID[name] BRACKET_L exprs[args] BRACKET_R
        {
          $$ = ASTfuncall($name);
          FUNCALL_FUN_ARGS($$) = $args;
        }
        | ID[name] BRACKET_L BRACKET_R
        {
          $$ = ASTfuncall($name);
        }
        ;

ids: ID
      {
        $$ = ASTids($1);
      }
   | ID COMMA ids
      {
        $$ = ASTids($1);
        IDS_NEXT($$) = $3;
      }
      ;

opt_exprs: exprs { $$ = $1; } | /* Empty */ { $$ = NULL; };

vardecls: vardecl vardecls
        {
          $$ = $1;
          VARDECL_NEXT($1) = $2;
        }
        | vardecl
        {
          $$ = $1;
        }

vardecl: type[t] ID[name] SEMICOLON
        {
          $$ = ASTvardecl($name, $t);
        }
       | type[t] ID[name] LET expr[init] SEMICOLON
        {
          $$ = ASTvardecl($name, $t);
          VARDECL_INIT($$) = $init;
        }
        ;

varlet: ID[id] SBRACKET_L exprs[indices] SBRACKET_R
        {
          $$ = ASTvarlet($id);
          VARLET_INDICES($$) = $indices;
          AddLocToNode($$, &@1, &@1);
        }
      | ID[id]
        {
          $$ = ASTvarlet($id);
          AddLocToNode($$, &@1, &@1);
        }
        ;

var: ID[id] SBRACKET_L exprs[indices] SBRACKET_R
        {
          $$ = ASTvar($id);
          VAR_INDICES($$) = $indices;
          AddLocToNode($$, &@1, &@1);
        }
      | ID[id]
        {
          $$ = ASTvar($id);
          AddLocToNode($$, &@1, &@1);
        }
        ;

exprs: expr exprs { $$ = ASTexprs($1, $2); }  // Does expr exprs even exist?
     | expr COMMA exprs { $$ = ASTexprs($1, $3); }
     | expr { $$ = ASTexprs($1, NULL); }
     ;

/* EXPRESSIONS AND PRECEDENCE */

expr: preclvl6;

preclvl6: preclvl6 OR preclvl5 { $$ = ASTbinop($1, $3, BO_or); AddLocToNode($$, &@1, &@3); }
        | preclvl5
        ;

preclvl5: preclvl5 AND preclvl4 { $$ = ASTbinop($1, $3, BO_and); AddLocToNode($$, &@1, &@3); }
        | preclvl4
        ;

preclvl4: preclvl4 EQ preclvl3 { $$ = ASTbinop($1, $3, BO_eq); AddLocToNode($$, &@1, &@3); }
        | preclvl4 NE preclvl3 { $$ = ASTbinop($1, $3, BO_ne); AddLocToNode($$, &@1, &@3); }
        | preclvl4 LT preclvl3 { $$ = ASTbinop($1, $3, BO_lt); AddLocToNode($$, &@1, &@3); }
        | preclvl4 LE preclvl3 { $$ = ASTbinop($1, $3, BO_le); AddLocToNode($$, &@1, &@3); }
        | preclvl4 GT preclvl3 { $$ = ASTbinop($1, $3, BO_gt); AddLocToNode($$, &@1, &@3); }
        | preclvl4 GE preclvl3 { $$ = ASTbinop($1, $3, BO_ge); AddLocToNode($$, &@1, &@3); }
        | preclvl3
        ;

preclvl3: preclvl3 PLUS preclvl2 { $$ = ASTbinop($1, $3, BO_add); AddLocToNode($$, $1, $3); }
        | preclvl3 MINUS preclvl2 { $$ = ASTbinop($1, $3, BO_sub); AddLocToNode($$, $1, $3); }
        | preclvl2
        ;

preclvl2: preclvl2 STAR preclvl1 { $$ = ASTbinop($1, $3, BO_mul); AddLocToNode($$, $1, $3); }
        | preclvl2 SLASH preclvl1 { $$ = ASTbinop($1, $3, BO_div); AddLocToNode($$, $1, $3); }
        | preclvl2 PERCENT preclvl1 { $$ = ASTbinop($1, $3, BO_mod); AddLocToNode($$, $1, $3); }
        | preclvl1
        ;

preclvl1: MINUS primitive { $$ = ASTmonop($2, MO_neg); AddLocToNode($$, &@1, &@2); };
        | primitive
        ;

primitive: constants
         | var
         | funcall
         | BRACKET_L expr BRACKET_R { $$ = $2; }
         ;

/* END EXPRESSIONS AND PRECEDENCE */

constants: floatval
          {
            $$ = $1;
          }
        | intval
          {
            $$ = $1;
          }
        | boolval
          {
            $$ = $1;
          }
        ;

floatval: FLOAT
           {
             $$ = ASTfloat($1);
           }
         ;

intval: NUM
        {
          $$ = ASTnum($1);
        }
      ;

boolval: TRUEVAL
         {
           $$ = ASTbool(true);
         }
       | FALSEVAL
         {
           $$ = ASTbool(false);
         }
       ;

monop: NOT       { $$ = MO_not; }
     | MINUS     { $$ = MO_neg; }
     ;

binop: PLUS      { $$ = BO_add; }
     | MINUS     { $$ = BO_sub; }
     | STAR      { $$ = BO_mul; }
     | SLASH     { $$ = BO_div; }
     | PERCENT   { $$ = BO_mod; }
     | LE        { $$ = BO_le; }
     | LT        { $$ = BO_lt; }
     | GE        { $$ = BO_ge; }
     | GT        { $$ = BO_gt; }
     | EQ        { $$ = BO_eq; }
     | OR        { $$ = BO_or; }
     | AND       { $$ = BO_and; }
     ;

type: INTTYPE   { $$ = CT_int; }
    | FLOATTYPE { $$ = CT_float; }
    | BOOLTYPE  { $$ = CT_bool; }
    | VOIDTYPE  { $$ = CT_void; }
    ;

%%

void AddLocToNode(node_st *node, void *begin_loc, void *end_loc)
{
    // Needed because YYLTYPE unpacks later than top-level decl.
    YYLTYPE *loc_b = (YYLTYPE*)begin_loc;
    YYLTYPE *loc_e = (YYLTYPE*)end_loc;
    NODE_BLINE(node) = loc_b->first_line;
    NODE_BCOL(node) = loc_b->first_column;
    NODE_ELINE(node) = loc_e->last_line;
    NODE_ECOL(node) = loc_e->last_column;
}

int yyerror(char *error)
{
  CTI(CTI_ERROR, true, "line %d, col %d\nError parsing source code: %s\n",
            global.line, global.col, error);
  CTIabortOnError();
  return 0;
}

node_st *SPdoScanParse(node_st *root)
{
    DBUG_ASSERT(root == NULL, "Started parsing with existing syntax tree.");
    yyin = fopen(global.input_file, "r");
    if (yyin == NULL) {
        CTI(CTI_ERROR, true, "Cannot open file '%s'.", global.input_file);
        CTIabortOnError();
    }
    yyparse();
    return parseresult;
}
