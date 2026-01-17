#include "ast.h"
#include <stdlib.h>
#include <string.h>

/* Utility */
static astnode *new_node(asttype type) {
    astnode *n = malloc(sizeof(astnode));
    n->type = type;
    n->left = n->right = n->extra = NULL;
    n->name = NULL;
    n->value = 0;
    n->op = 0;
    return n;
}

/* Constructors */
astnode *make_num(int value) {
    astnode *n = new_node(AST_NUM);
    n->value = value;
    return n;
}

astnode *make_id(char *name) {
    astnode *n = new_node(AST_ID);
    n->name = strdup(name);
    return n;
}

astnode *make_op(char op, astnode *l, astnode *r) {
    astnode *n = new_node(AST_OP);
    n->op = op;
    n->left = l;
    n->right = r;
    return n;
}

astnode *make_assign(astnode *id, astnode *expr) {
    astnode *n = new_node(AST_ASSIGN);
    n->left = id;
    n->right = expr;
    return n;
}

astnode *make_read(astnode *id) {
    astnode *n = new_node(AST_READ);
    n->left = id;
    return n;
}

astnode *make_write(astnode *expr) {
    astnode *n = new_node(AST_WRITE);
    n->left = expr;
    return n;
}

astnode *make_if(astnode *cond, astnode *then_part, astnode *else_part) {
    astnode *n = new_node(AST_IF);
    n->left = cond;
    n->right = then_part;
    n->extra = else_part;
    return n;
}

astnode *make_repeat(astnode *body, astnode *cond) {
    astnode *n = new_node(AST_REPEAT);
    n->left = body;
    n->right = cond;
    return n;
}

astnode *make_seq(astnode *first, astnode *second) {
    if (!first) return second;
    astnode *n = new_node(AST_SEQ);
    n->left = first;
    n->right = second;
    return n;
}

/* Debug printing */
static void indent(int n) {
    for (int i = 0; i < n; i++) printf("  ");
}

void print_ast(astnode *t, int level) {
    if (!t) return;

    indent(level);
    switch (t->type) {
        case AST_NUM:    printf("NUM(%d)\n", t->value); break;
        case AST_ID:     printf("ID(%s)\n", t->name); break;
        case AST_OP:     printf("OP(%c)\n", t->op); break;
        case AST_ASSIGN: printf("ASSIGN\n"); break;
        case AST_READ:   printf("READ\n"); break;
        case AST_WRITE:  printf("WRITE\n"); break;
        case AST_IF:     printf("IF\n"); break;
        case AST_REPEAT: printf("REPEAT\n"); break;
        case AST_SEQ:    printf("SEQ\n"); break;
    }

    print_ast(t->left, level + 1);
    print_ast(t->right, level + 1);
    print_ast(t->extra, level + 1);
}

