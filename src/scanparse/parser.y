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
node_st* reverse_vardecls(node_st* head);


%}

%union {
 char               *id;
 int                 cint;
 float               cflt;
 bool               cbool;
 enum Type          ctype;
 node_st             *node;
}

%locations

%token BRACE_L BRACE_R BRACKET_L BRACKET_R SBRACKET_L SBRACKET_R COMMA SEMICOLON
%token IF ELSE WHILE DO FOR RETURN
%token MINUS PLUS STAR SLASH PERCENT LE LT GE GT EQ NE OR AND
%token NOT UMINUS CAST
%token TRUEVAL FALSEVAL LET

%token <cint> NUM
%token <cflt> FLOAT
%token <id> ID
%token INTTYPE FLOATTYPE BOOLTYPE VOIDTYPE
%token EXPORT EXTERN

// Precendence
%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left STAR SLASH PERCENT
%right NOT CAST
%nonassoc UMINUS

%type <node> program decls expr exprs ids exprstmt
%type <node> returnstmt funcall fundef funbody
%type <node> ifstmt whilestmt dowhilestmt forstmt
%type <node> globdecl globdef param vardecl stmts
%type <node> assign varlet var
%type <node> intval floatval boolval
%type <ctype> type
%type <node> constants decl stmt

%type <node> opt_expr_indices
%type <node> opt_params param_list
%type <node> vardecls localfundef localfundefs
%type <cbool> opt_export_bool


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

globdef: opt_export_bool[export] type[t] ID[name] LET expr[init] SEMICOLON
        {
          $$ = ASTglobdef($name, $t);
          GLOBDEF_INIT($$) = $init;
          GLOBDEF_EXPORT($$) = $export;
          AddLocToNode($$, &@t, &@init);
        }
       | opt_export_bool[export] type[t] SBRACKET_L exprs[dims] SBRACKET_R ID[name] LET expr[init] SEMICOLON
        {
          $$ = ASTglobdef($name, $t);
          GLOBDEF_DIMS($$) = $dims;
          GLOBDEF_INIT($$) = $init;
          GLOBDEF_EXPORT($$) = $export;
          AddLocToNode($$, &@t, &@init);
        }
        ;

opt_export_bool: EXPORT { $$ = true; }
               | %empty { $$ = false; };

stmts: stmt stmts
        {
          $$ = ASTstmts($1, $2);
        }
      | %empty
        {
          $$ = NULL;
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

opt_params: param_list { $$ = $1; }
          | %empty { $$ = NULL; };

param_list: param { $$ = $1; }
          | param_list COMMA param
            {
              $$ = $1;
              PARAM_NEXT($1) = $3;
            }
            ;

fundef: opt_export_bool[export] type[t] ID[name] BRACKET_L opt_params[params] BRACKET_R funbody[body]
        {
          $$ = ASTfundef($name, $t);
          FUNDEF_PARAMS($$) = $params;
          FUNDEF_BODY($$) = $body;
          FUNDEF_EXPORT($$) = $export;
          FUNDEF_IS_EXTERN($$) = false;
          AddLocToNode($$, &@t, &@name);
        }
      | EXTERN type[t] ID[name] BRACKET_L opt_params[params] BRACKET_R
        {
          $$ = ASTfundef($name, $t);
          FUNDEF_PARAMS($$) = $params;
          FUNDEF_BODY($$) = NULL;
          FUNDEF_IS_EXTERN($$) = true;
          AddLocToNode($$, &@1, &@6);
        }
        ;

localfundef: type[t] ID[name] BRACKET_L opt_params[params] BRACKET_R funbody[body]
        {
          $$ = ASTfundef($name, $t);
          FUNDEF_PARAMS($$) = $params;
          FUNDEF_BODY($$) = $body;
          FUNDEF_EXPORT($$) = false;
          AddLocToNode($$, &@t, &@name);
        }
        ;

localfundefs: localfundef localfundefs
         {
          $$ = ASTfundefs($1);
          FUNDEFS_NEXT($$) = $2;
         }
       | %empty
         {
          $$ = NULL;
         }
         ;

funbody: BRACE_L[startbrace] vardecls[decls] localfundefs[fundefs] stmts[sts] BRACE_R[endbrace]
          {
            $$ = ASTfunbody();
            FUNBODY_DECLS($$) = reverse_vardecls($decls);
            FUNBODY_LOCAL_FUNDEFS($$) = $fundefs;
            FUNBODY_STMTS($$) = $sts;
            AddLocToNode($$, &@startbrace, &@endbrace);
          }
          ;

param: type[t] ID[id] SBRACKET_L ids[dims] SBRACKET_R
      {
        $$ = ASTparam($id, $t);
        PARAM_DIMS($$) = $dims;
        AddLocToNode($$, &@t, &@5);
      }
      | type[t] ID[id]
      {
        $$ = ASTparam($id, $t);
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

forstmt: FOR BRACKET_L INTTYPE ID[var] LET expr[init] COMMA expr[stop] COMMA expr[step] BRACKET_R BRACE_L stmts[block] BRACE_R
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

// Vardecls list needs to be generated in reverse to avoid conflicts
vardecls: vardecls vardecl
        {
          $$ = $2;
          VARDECL_NEXT($$) = $1;
        }
        | %empty
        {
          $$ = NULL;
        }

vardecl: type[t] ID[name] opt_expr_indices[indices] SEMICOLON
        {
          $$ = ASTvardecl($name, $t);
          VARDECL_DIMS($$) = $indices;
        }
       | type[t] ID[name] opt_expr_indices[indices] LET expr[init] SEMICOLON
        {
          $$ = ASTvardecl($name, $t);
          VARDECL_INIT($$) = $init;
          VARDECL_DIMS($$) = $indices;
        }
        ;

varlet: ID[id] opt_expr_indices[indices]
        {
          $$ = ASTvarlet($id);
          VARLET_INDICES($$) = $indices;
          AddLocToNode($$, &@1, &@1);
        }
        ;

var: ID[id] opt_expr_indices[indices]
        {
          $$ = ASTvar($id);
          VAR_INDICES($$) = $indices;
          AddLocToNode($$, &@1, &@1);
        }
        ;

opt_expr_indices: SBRACKET_L exprs[e] SBRACKET_R
                  {
                    $$ = $e;
                  }
                | %empty
                  {
                    $$ = NULL;
                  }
                  ;

exprs: expr COMMA exprs { $$ = ASTexprs($1, $3); }
     | expr { $$ = ASTexprs($1, NULL); }
     ;

expr: constants
    | var
    | funcall
    | BRACKET_L expr[e] BRACKET_R { $$ = $e; }
    | SBRACKET_L exprs[es] SBRACKET_R { $$ = ASTarrexpr($es); }
    | BRACKET_L type[t] BRACKET_R expr[e] %prec CAST { $$ = ASTcast($e, $t); }
    | expr[left] OR expr[right] { $$ = ASTbinop($1, $3, BO_or); AddLocToNode($$, &@1, &@3); }
    | expr[left] AND expr[right] { $$ = ASTbinop($1, $3, BO_and); AddLocToNode($$, &@1, &@3); }
    | expr[left] EQ expr[right] { $$ = ASTbinop($1, $3, BO_eq); AddLocToNode($$, &@1, &@3); }
    | expr[left] NE expr[right] { $$ = ASTbinop($1, $3, BO_ne); AddLocToNode($$, &@1, &@3); }
    | expr[left] LT expr[right] { $$ = ASTbinop($1, $3, BO_lt); AddLocToNode($$, &@1, &@3); }
    | expr[left] LE expr[right] { $$ = ASTbinop($1, $3, BO_le); AddLocToNode($$, &@1, &@3); }
    | expr[left] GT expr[right] { $$ = ASTbinop($1, $3, BO_gt); AddLocToNode($$, &@1, &@3); }
    | expr[left] GE expr[right] { $$ = ASTbinop($1, $3, BO_ge); AddLocToNode($$, &@1, &@3); }
    | expr[left] PLUS expr[right] { $$ = ASTbinop($1, $3, BO_add); AddLocToNode($$, &@1, &@3); }
    | expr[left] MINUS expr[right] { $$ = ASTbinop($1, $3, BO_sub); AddLocToNode($$, &@1, &@3); }
    | expr[left] STAR expr[right] { $$ = ASTbinop($1, $3, BO_mul); AddLocToNode($$, &@1, &@3); }
    | expr[left] SLASH expr[right] { $$ = ASTbinop($1, $3, BO_div); AddLocToNode($$, &@1, &@3); }
    | expr[left] PERCENT expr[right] { $$ = ASTbinop($1, $3, BO_mod); AddLocToNode($$, &@1, &@3); }
    | MINUS expr[right] %prec UMINUS { $$ = ASTmonop($2, MO_neg); AddLocToNode($$, &@1, &@2); }
    | NOT expr[right] { $$ = ASTmonop($2, MO_not); AddLocToNode($$, &@1, &@2); }
    ;

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

type: INTTYPE   { $$ = CT_int; }
    | FLOATTYPE { $$ = CT_float; }
    | BOOLTYPE  { $$ = CT_bool; }
    | VOIDTYPE  { $$ = CT_void; }
    ;

%%

node_st* reverse_vardecls(node_st* head) {
  node_st* prev = NULL;
  node_st* current = head;
  node_st* next;

  while (current != NULL) {
    next = VARDECL_NEXT(current);
    VARDECL_NEXT(current) = prev;
    prev = current;
    current = next;
  }
  
  return prev;
}

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
