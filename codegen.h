// TODO: This file contains all the declarations for the codegeneration.
/* The code emitted is only a target-independent assembly code. It doesn't correspond to any instruction set of a microprocessor. */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"
#include "ast.h"
#include<stdio.h>
#include<stdlib.h>

void generate_code(IRInstruction *ir);
/*Generate the assembly code. */

#endif
