#ifndef AST_H
#define AST_H

#include <stdio.h>

/* AST node types */
typedef enum {
    AST_NUM,
    AST_ID,
    AST_ASSIGN,
    AST_OP,
    AST_READ,
    AST_WRITE,
    AST_IF,
    AST_REPEAT,
    AST_SEQ
} asttype;

/* AST node structure */
typedef struct astnode {
    asttype type;

    int value;              // for numbers
    char *name;             // for identifiers
    char op;                // for operators

    struct astnode *left;
    struct astnode *right;
    struct astnode *extra;  // used for else / until condition
} astnode;

/* Constructors */
astnode *make_num(int value);
astnode *make_id(char *name);
astnode *make_op(char op, astnode *l, astnode *r);
astnode *make_assign(astnode *id, astnode *expr);
astnode *make_read(astnode *id);
astnode *make_write(astnode *expr);
astnode *make_if(astnode *cond, astnode *then_part, astnode *else_part);
astnode *make_repeat(astnode *body, astnode *cond);
astnode *make_seq(astnode *first, astnode *second);

/* Debug */
void print_ast(astnode *t, int indent);

#endif

