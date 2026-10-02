#ifndef CODEGEN_VM_H
#define CODEGEN_VM_H

#include "ir.h"

/* Walks the (already optimized, if applicable) IR list and writes a
 * .tbc bytecode image to out_path, ready for vm.c to execute.
 *
 * Returns 0 on success, -1 on failure (e.g. too many variables/labels,
 * or the output file couldn't be written). */
int generate_bytecode(IRInstruction *ir, const char *out_path);

#endif /* CODEGEN_VM_H */
