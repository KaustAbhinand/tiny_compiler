#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "ast.h"

/* Global IR list */
IRInstruction *ir_head = NULL;
IRInstruction *ir_tail = NULL;

/* Counters for temps and labels */
static int temp_counter = 0;
static int label_counter = 0;

/* Generate new temporary variable */
char *new_temp() {
    char *temp = malloc(16);
    sprintf(temp, "t%d", temp_counter++);
    return temp;
}

/* Generate new label */
char *new_label() {
    char *label = malloc(16);
    sprintf(label, "L%d", label_counter++);
    return label;
}

/* Emit an IR instruction */
void emit(IRType type, const char *result, const char *arg1, const char *arg2, 
          const char *relop, const char *label) {
    IRInstruction *instr = malloc(sizeof(IRInstruction));
    instr->type = type;
    instr->next = NULL;
    
    /* Copy strings safely */
    if (result) strncpy(instr->result, result, 31);
    else instr->result[0] = '\0';
    
    if (arg1) strncpy(instr->arg1, arg1, 31);
    else instr->arg1[0] = '\0';
    
    if (arg2) strncpy(instr->arg2, arg2, 31);
    else instr->arg2[0] = '\0';
    
    if (relop) strncpy(instr->relop, relop, 7);
    else instr->relop[0] = '\0';
    
    if (label) strncpy(instr->label, label, 31);
    else instr->label[0] = '\0';
    
    /* Append to list */
    if (!ir_head) {
        ir_head = ir_tail = instr;
    } else {
        ir_tail->next = instr;
        ir_tail = instr;
    }
}

/* Forward declarations */
static char *gen_expression(astnode *node);
static void gen_statement(astnode *node);

/* Generate code for expressions - returns temp holding result */
static char *gen_expression(astnode *node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_NUM: {
            char *temp = new_temp();
            char value[32];
            sprintf(value, "%d", node->value);
            emit(IR_ASSIGN, temp, value, NULL, NULL, NULL);
            return temp;
        }
        
        case AST_ID: {
            char *temp = new_temp();
            emit(IR_ASSIGN, temp, node->name, NULL, NULL, NULL);
            return temp;
        }
        
        case AST_OP: {
            char *left_temp = gen_expression(node->left);
            char *right_temp = gen_expression(node->right);
            char *result_temp = new_temp();
            
            IRType op_type;
            switch (node->op) {
                case '+': op_type = IR_ADD; break;
                case '-': op_type = IR_SUB; break;
                case '*': op_type = IR_MUL; break;
                case '/': op_type = IR_DIV; break;
                case '<': 
                case '>': 
                case '=': 
                case '!': 
                case 'l': 
                case 'g': {
                    /* For comparison operators, generate comparison result */
                    char relop_str[8];
                    switch (node->op) {
                        case '<': strcpy(relop_str, "<"); break;
                        case '>': strcpy(relop_str, ">"); break;
                        case '=': strcpy(relop_str, "=="); break;
                        case '!': strcpy(relop_str, "!="); break;
                        case 'l': strcpy(relop_str, "<="); break;
                        case 'g': strcpy(relop_str, ">="); break;
                    }
                    
                    /* Generate: result = arg1 relop arg2 */
                    emit(IR_CMP, result_temp, left_temp, right_temp, relop_str, NULL);
                    return result_temp;
                }
                default: 
                    op_type = IR_ASSIGN; 
                    break;
            }
            
            emit(op_type, result_temp, left_temp, right_temp, NULL, NULL);
            return result_temp;
        }
        
        default:
            return NULL;
    }
}

/* Generate code for conditional - used in IF and REPEAT */
static void gen_condition(astnode *cond, char *true_label, char *false_label) {
    if (!cond) return;
    
    if (cond->type == AST_OP) {
        char *left_temp = gen_expression(cond->left);
        char *right_temp = gen_expression(cond->right);
        
        char relop[8];
        switch (cond->op) {
            case '<': strcpy(relop, "<"); break;
            case '>': strcpy(relop, ">"); break;
            case '=': strcpy(relop, "=="); break;
            case '!': strcpy(relop, "!="); break;
            case 'l': strcpy(relop, "<="); break;
            case 'g': strcpy(relop, ">="); break;
            default: strcpy(relop, "=="); break;
        }
        
        if (true_label) {
            /* if condition goto true_label */
            emit(IR_IF, NULL, left_temp, right_temp, relop, true_label);
        }
        
        if (false_label) {
            /* Generate opposite condition for false branch */
            char neg_relop[8];
            switch (cond->op) {
                case '<': strcpy(neg_relop, ">="); break;
                case '>': strcpy(neg_relop, "<="); break;
                case '=': strcpy(neg_relop, "!="); break;
                case '!': strcpy(neg_relop, "=="); break;
                case 'l': strcpy(neg_relop, ">"); break;
                case 'g': strcpy(neg_relop, "<"); break;
                default: strcpy(neg_relop, "!="); break;
            }
            emit(IR_IF, NULL, left_temp, right_temp, neg_relop, false_label);
        }
    } else {
        /* Non-comparison condition: check if != 0 */
        char *temp = gen_expression(cond);
        if (true_label) {
            emit(IR_IF, NULL, temp, "0", "!=", true_label);
        }
        if (false_label) {
            emit(IR_IF, NULL, temp, "0", "==", false_label);
        }
    }
}

/* Generate code for statements */
static void gen_statement(astnode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_ASSIGN: {
            char *expr_temp = gen_expression(node->right);
            char *var = node->left->name;
            emit(IR_ASSIGN, var, expr_temp, NULL, NULL, NULL);
            break;
        }
        
        case AST_READ: {
            char *var = node->left->name;
            emit(IR_READ, var, NULL, NULL, NULL, NULL);
            break;
        }
        
        case AST_WRITE: {
            char *expr_temp = gen_expression(node->left);
            emit(IR_WRITE, expr_temp, NULL, NULL, NULL, NULL);
            break;
        }
        
        case AST_IF: {
            char *else_label = new_label();
            char *end_label = new_label();
            
            /* Generate condition - jump to else if false */
            gen_condition(node->left, NULL, else_label);
            
            /* Then part */
            gen_statement(node->right);
            
            if (node->extra) {
                /* Has else part */
                emit(IR_GOTO, NULL, NULL, NULL, NULL, end_label);
                emit(IR_LABEL, NULL, NULL, NULL, NULL, else_label);
                gen_statement(node->extra);
                emit(IR_LABEL, NULL, NULL, NULL, NULL, end_label);
            } else {
                /* No else part */
                emit(IR_LABEL, NULL, NULL, NULL, NULL, else_label);
            }
            break;
        }
        
        case AST_REPEAT: {
	    printf("DEBUG: REPEAT op CHARACTER: %c\n", node->right->op);
            char *start_label = new_label();
            
            /* Start of loop */
            emit(IR_LABEL, NULL, NULL, NULL, NULL, start_label);
            
            /* Loop body */
            gen_statement(node->left);
            
            /* Condition: repeat UNTIL condition is true */
            /* So continue loop while condition is FALSE */
            /* Jump back if condition is false */
            if (node->right->type == AST_OP) {
                char *left_temp = gen_expression(node->right->left);
                char *right_temp = gen_expression(node->right->right);
                
                /* Negate the condition for repeat-until */
                char relop[8];
                switch (node->right->op) {
                    case '<': strcpy(relop, ">="); break;  /* continue if >= (not <) */
                    case '>': strcpy(relop, "<="); break;  /* continue if <= (not >) */
                    case '=': strcpy(relop, "!="); break;  /* continue if != (not =) */
                    case '!': strcpy(relop, "=="); break;  /* continue if == (not !=) */
                    case 'l': strcpy(relop, ">"); break;   /* continue if > (not <=) */
                    case 'g': strcpy(relop, "<"); break;   /* continue if < (not >=) */
                    default: strcpy(relop, "!="); break;
                }
                
                emit(IR_IF, NULL, left_temp, right_temp, relop, start_label);
            }
            break;
        }
        
        case AST_SEQ: {
            gen_statement(node->left);
            gen_statement(node->right);
            break;
        }
        
        default:
            break;
    }
}

/* Main entry point */
void generate_ir(astnode *root) {
    /* Reset counters */
    temp_counter = 0;
    label_counter = 0;
    ir_head = ir_tail = NULL;
    
    /* Generate IR from AST */
    gen_statement(root);
}

/* Print IR instructions */
void print_ir() {
    printf("\n--------Intermidiate Representation:----------\n");
    
    for (IRInstruction *instr = ir_head; instr; instr = instr->next) {
        switch (instr->type) {
            case IR_ASSIGN:
                printf("%s = %s\n", instr->result, instr->arg1);
                break;
                
            case IR_ADD:
                printf("%s = %s + %s\n", instr->result, instr->arg1, instr->arg2);
                break;
                
            case IR_SUB:
                printf("%s = %s - %s\n", instr->result, instr->arg1, instr->arg2);
                break;
                
            case IR_MUL:
                printf("%s = %s * %s\n", instr->result, instr->arg1, instr->arg2);
                break;
                
            case IR_DIV:
                printf("%s = %s / %s\n", instr->result, instr->arg1, instr->arg2);
                break;
                
            case IR_CMP:
                printf("%s = %s %s %s\n", instr->result, instr->arg1, 
                       instr->relop, instr->arg2);
                break;
                
            case IR_LABEL:
                printf("%s:\n", instr->label);
                break;
                
            case IR_GOTO:
                printf("goto %s\n", instr->label);
                break;
                
            case IR_IF:
                printf("if %s %s %s goto %s\n", instr->arg1, instr->relop, 
                       instr->arg2, instr->label);
                break;
                
            case IR_READ:
                printf("read %s\n", instr->result);
                break;
                
            case IR_WRITE:
                printf("write %s\n", instr->result);
                break;
        }
    }
}
