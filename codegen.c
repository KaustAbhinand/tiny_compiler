/* TODO: Contains the implementation of the function: generate_code. */

#include "ir.h"
#include "ast.h"
#include "codegen.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int is_number(const char *str) {
      if (!str || !str[0]) return 0;
    if (str[0] == '-' || str[0] == '+') str++;
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}

void generate_instr(IRInstruction *instr) {
     switch (instr->type) {
        case IR_ASSIGN:
            if (is_number(instr->arg1)) {
                /* Load immediate value */
                printf("    LOADI %s, %s\n", instr->result, instr->arg1);
            } else {
                /* Copy from variable/temp */
                printf("    LOAD %s, %s\n", instr->result, instr->arg1);
            }
            break;
            
        case IR_ADD:
            printf("    ADD %s, %s, %s\n", instr->result, instr->arg1, instr->arg2);
            break;
            
        case IR_SUB:
            printf("    SUB %s, %s, %s\n", instr->result, instr->arg1, instr->arg2);
            break;
            
        case IR_MUL:
            printf("    MUL %s, %s, %s\n", instr->result, instr->arg1, instr->arg2);
            break;
            
        case IR_DIV:
            printf("    DIV %s, %s, %s\n", instr->result, instr->arg1, instr->arg2);
            break;
            
        case IR_CMP:
            /* Comparison: result = arg1 relop arg2 */
            /* Generate compare and set result to 0 or 1 */
            printf("    CMP %s, %s, %s    ; %s %s %s\n", 
                   instr->result, instr->arg1, instr->arg2,
                   instr->arg1, instr->relop, instr->arg2);
            break;
            
        case IR_LABEL:
            printf("%s:\n", instr->label);
            break;
            
        case IR_GOTO:
            printf("    JUMP %s\n", instr->label);
            break;
            
        case IR_IF: {
            /* Conditional jump based on relational operator */
            const char *jump_instr;
            if (strcmp(instr->relop, "<") == 0) {
                jump_instr = "JLT";
            } else if (strcmp(instr->relop, ">") == 0) {
                jump_instr = "JGT";
            } else if (strcmp(instr->relop, "==") == 0) {
                jump_instr = "JEQ";
            } else if (strcmp(instr->relop, "!=") == 0) {
                jump_instr = "JNE";
            } else if (strcmp(instr->relop, "<=") == 0) {
                jump_instr = "JLE";
            } else if (strcmp(instr->relop, ">=") == 0) {
                jump_instr = "JGE";
            } else {
                jump_instr = "JEQ";  // default
            }
            
            printf("    %s %s, %s, %s\n", jump_instr, instr->arg1, instr->arg2, instr->label);
            break;
        }
            
        case IR_READ:
            printf("    READ %s\n", instr->result);
            break;
            
        case IR_WRITE:
            printf("    WRITE %s\n", instr->result);
            break;
    }
}

void generate_code(IRInstruction *ir) {

        printf("\n========== Pseudo-Assembly Code ==========\n");
    printf("; Target-Independent Assembly\n");
    
    /* Generate code for each IR instruction */
    for (IRInstruction *instr = ir; instr; instr = instr->next) {
        generate_instr(instr);
    }
    
    /* Add halt at the end */
    printf("\n    HALT\n");
}

