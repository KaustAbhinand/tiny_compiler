%code requires {
  #include "ast.h"
}

%{

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern int yylex();
void yyerror(const char *s);

astnode *root;
%}

/* YYSTYPE */
%union {
    int num;
    char *str;
    astnode *node;
}

/* Tokens from scanner */
%token IF THEN ELSE END REPEAT UNTIL READ WRITE SEMI
%token PLUS MINUS MUL DIV
%token LT GT LTE GTE EQ NE
%token ASSIGN
%token LPAREN RPAREN
%token <num> NUMBER
%token <str> IDENTIFIER

/* Non-terminals */
%type <node> program stmt_seq stmt_term simple_stmt struct_stmt exp simple_exp term factor

%%

program
    : stmt_seq END              { root = $1; }
    ;

stmt_seq
    : stmt_term                   { $$ = $1; }
    | stmt_seq stmt_term      { $$ = make_seq($1, $2); }
    ;
stmt_term :
	  simple_stmt SEMI        { $$ = $1; }
   |      struct_stmt             { $$ = $1; }
   ;

simple_stmt:
	   IDENTIFIER ASSIGN exp { $$ = make_assign(make_id($1),$3); }
   |       READ IDENTIFIER       { $$ = make_read(make_id($2));}
   |       WRITE exp      { $$ = make_write($2);}
   ;

struct_stmt:
	   IF exp THEN stmt_seq END { $$ = make_if($2, $4, NULL);}
   |       IF exp THEN stmt_seq ELSE stmt_seq END  { $$ = make_if($2, $4, $6);}
   |       REPEAT stmt_seq UNTIL exp SEMI { $$ = make_repeat($2, $4); }
   ; 


exp
    : simple_exp             { $$ = $1; }
    | simple_exp LT simple_exp  { $$ = make_op('<', $1, $3); }
    | simple_exp GT simple_exp  { $$ = make_op('>', $1, $3); }
    | simple_exp EQ simple_exp  { $$ = make_op('=', $1, $3); }
    | simple_exp NE simple_exp  { $$ = make_op('!', $1, $3); }
    | simple_exp LTE simple_exp { $$ = make_op('l', $1, $3); }
    | simple_exp GTE simple_exp { $$ = make_op('g', $1, $3); }
    ;

simple_exp
    : term                   { $$ = $1; }
    | simple_exp PLUS term   { $$ = make_op('+', $1, $3); }
    | simple_exp MINUS term  { $$ = make_op('-', $1, $3); }
    ;

term
    : factor                 { $$ = $1; }
    | term MUL factor        { $$ = make_op('*', $1, $3); }
    | term DIV factor        { $$ = make_op('/', $1, $3); }
    ;

factor
    : NUMBER                 { $$ = make_num($1); }
    | IDENTIFIER             { $$ = make_id($1); }
    | LPAREN exp RPAREN      { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}

