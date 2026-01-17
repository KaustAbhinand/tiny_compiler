/*TODO: This is the scanner for the compiler, written in lex. */
%option yylineno

%{
#include "parser.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

/* definitions */
digit       [0-9]
letter      [a-zA-Z]
identifier  {letter}({letter}|{digit})*
number      {digit}+
whitespace  [ \t\r\n]+
comment     \{[^}]*\}

%%


"if"        { return IF; }
"then"      { return THEN; }
"else"      { return ELSE; }
"read"      { return READ; }
"write"     { return WRITE; }
"repeat"    { return REPEAT; }
"until"     { printf("DEBUG: UNTIL\n");return UNTIL; }
"end"       { printf("DEBUG: END\n");return END; }


":="        { printf("DEBUG: ASSIGN\n"); return ASSIGN; }
"+"         { printf("DEBUG: PLUS\n"); return PLUS; }
"-"         { printf("DEBUG: MINUS\n");return MINUS; }
"*"         { printf("DEBUG: MUL\n");return MUL; }
"/"         { printf("DEBUG: DIV\n");return DIV; }


"<"         { printf("DEBUG: LT\n"); return LT; }
">"         { printf("DEBUG: GT\n"); return GT; } 
"<="        { printf("DEBUG: LTE\n"); return LTE; }
">="        { printf("DEBUG: GTE\n"); return GTE; }
"=="        { printf("DEBUG: EQ\n"); return EQ; }
"!="        { printf("DEBUG: NE\n"); return NE; }


"("         { printf("DEBUG: LPAREN\n");return LPAREN; }
")"         { printf("DEBUG: RPAREN\n");return RPAREN; }
";"         { printf("DEBUG: SEMI\n");return SEMI; }


{number} {
    yylval.num = atoi(yytext);
    printf("DEBUG: NUMBER(%d)\n", yylval.num);
    return NUMBER;
}

{identifier} {
    yylval.str = strdup(yytext);
    printf("DEBUG: IDENTIFIER(%s)\n", yylval.str);
    return IDENTIFIER;
}


{comment}    { /* ignore */ }
{whitespace} { /* ignore */ }


. {
    fprintf(stderr, "unknown character at %d line: %s\n", yylineno,  yytext);
    return 0;
}

%%

int yywrap(void) {
    return 1;
}


