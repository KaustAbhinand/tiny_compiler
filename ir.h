#ifndef IR_H
#define IR_H

#include <stdio.h>
#include "ast.h"
/* Enum representing different IR instruction types */
typedef enum {
    IR_ASSIGN,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_CMP,
    IR_IF,
    IR_IF_GOTO,
    IR_GOTO,
    IR_LABEL,
    IR_READ,
    IR_WRITE
} IRType;

/* Structure representing a single IR instruction */
typedef struct ir_instruction {
    IRType type;                     // Type of IR instruction
    char result[32];                 // Destination (lhs)
    char arg1[32];                   // First operand
    char arg2[32];                   // Second operand
    char relop[8];                   // Relational operator (for IF)
    char label[32];                  // Label name
    struct ir_instruction *next;     // Pointer to next instruction
} IRInstruction;

/* Head and tail of IR instruction list */
extern IRInstruction *ir_head;
extern IRInstruction *ir_tail;

/* Function declarations */
void generate_ir(astnode *root);
void emit(IRType type, const char *result, const char *arg1, const char *arg2, const char *relop, const char *label);
void print_ir();
char *new_temp();
char *new_label();

#endif

