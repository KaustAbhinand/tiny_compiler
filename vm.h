#ifndef VM_H
#define VM_H

#include <stdint.h>
#include "bytecode.h"

/* Main memory and registers — exposed so tests/tools can inspect state */
extern uint16_t mem[MEM_SIZE];
extern uint16_t reg[RCNT];

/* Load a .tbc file into memory starting at PC_START.
 * Returns the number of words loaded, or -1 on failure. */
int ld_img(const char *fname);

/* Run until a HALT trap (or an unrecoverable error) stops execution. */
void run(void);





#endif /* VM_H */
