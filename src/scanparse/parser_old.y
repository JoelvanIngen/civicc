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
 enum BinOpType     cbinop;
 node_st             *node;
}

%locations

%token BRACKET_L BRACKET_R COMMA SEMICOLON
%token MINUS PLUS STAR SLASH PERCENT LE LT GE GT EQ NE OR AND
%token TRUEVAL FALSEVAL LET

%token <cint> NUM
%token <cflt> FLOAT
%token <id> ID

// Precedence low to high
%nonassoc LET          // Lowest precedence
%left OR               // Logical OR
%left AND              // Logical AND
%left EQ NE            // Equality and inequality
%left LT LE GT GE      // Relational operators
%left PLUS MINUS       // Additive operators
%left STAR SLASH PERCENT // Multiplicative operators

%type <node> intval floatval boolval constant expr
%type <node> stmts stmt assign varlet program
// %type <cbinop> binop  // Remove if binops are removed

%start program

%%

program: stmts
         {
           parseresult = ASTprogram($1);
         }
         ;

stmts: stmt stmts
        {
          $$ = ASTstmts($1, $2);
        }
      | stmt
        {
          $$ = ASTstmts($1, NULL);
        }
        ;

stmt: assign
       {
         $$ = $1;
       }
       ;

assign: varlet LET expr SEMICOLON
        {
          $$ = ASTassign($1, $3);
        }
        ;

varlet: ID
        {
          $$ = ASTvarlet($1);
          AddLocToNode($$, &@1, &@1);
        }
        ;

/*
// I think I should remove the entire binop because it seems to not work with the precedence rules?
binop: PLUS     { $$ = BO_add; }
     | MINUS    { $$ = BO_sub; }
     | STAR     { $$ = BO_mul; }
     | SLASH    { $$ = BO_div; }
     | PERCENT  { $$ = BO_mod; }
     | LE       { $$ = BO_le; }
     | LT       { $$ = BO_lt; }
     | GE       { $$ = BO_ge; }
     | GT       { $$ = BO_gt; }
     | EQ       { $$ = BO_eq; }
     | OR       { $$ = BO_or; }
     | AND      { $$ = BO_and; }
*/

expr: constant
      {
        $$ = $1;
      }
    | ID
      {
        $$ = ASTvar($1);
      }
    | BRACKET_L expr BRACKET_R
      {
        $$ = $2;
      }
    | expr PLUS expr
      {
        $$ = ASTbinop($1, $3, BO_add);
        AddLocToNode($$, &@1, &@3);
      }
    | expr MINUS expr
      {
        $$ = ASTbinop($1, $3, BO_sub);
        AddLocToNode($$, &@1, &@3);
      }
    | expr STAR expr
      {
        $$ = ASTbinop($1, $3, BO_mul);
        AddLocToNode($$, &@1, &@3);
      }
    | expr SLASH expr
      {
        $$ = ASTbinop($1, $3, BO_div);
        AddLocToNode($$, &@1, &@3);
      }
    | expr PERCENT expr
      {
        $$ = ASTbinop($1, $3, BO_mod);
        AddLocToNode($$, &@1, &@3);
      }
    | expr LE expr
      {
        $$ = ASTbinop($1, $3, BO_le);
        AddLocToNode($$, &@1, &@3);
      }
    | expr LT expr
      {
        $$ = ASTbinop($1, $3, BO_lt);
        AddLocToNode($$, &@1, &@3);
      }
    | expr GE expr
      {
        $$ = ASTbinop($1, $3, BO_ge);
        AddLocToNode($$, &@1, &@3);
      }
    | expr GT expr
      {
        $$ = ASTbinop($1, $3, BO_gt);
        AddLocToNode($$, &@1, &@3);
      }
    | expr EQ expr
      {
        $$ = ASTbinop($1, $3, BO_eq);
        AddLocToNode($$, &@1, &@3);
      }
    | expr NE expr
      {
        $$ = ASTbinop($1, $3, BO_ne);
        AddLocToNode($$, &@1, &@3);
      }
    | expr OR expr
      {
        $$ = ASTbinop($1, $3, BO_or);
        AddLocToNode($$, &@1, &@3);
      }
    | expr AND expr
      {
        $$ = ASTbinop($1, $3, BO_and);
        AddLocToNode($$, &@1, &@3);
      }
    ;

constant: floatval
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
