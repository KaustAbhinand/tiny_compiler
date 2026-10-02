#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

/* ============================================================
 * TINY Virtual Machine — Bytecode Format
 * ============================================================
 * Single source of truth for the instruction encoding, shared
 * between codegen_vm.c (emits .tbc files) and vm.c (executes
 * them). Modeled on the LC-3-style von Neumann VM described in
 * https://www.andreinc.net/2021/12/01/writing-a-simple-vm-in-less-than-125-lines-of-c/
 *
 * Word size: 16 bits (uint16_t), same as main memory.
 * Instruction format: 4-bit opcode + 12 bits of operands.
 *
 *   15            12 11                                    0
 *  +----------------+--------------------------------------+
 *  |     OPCODE     |              OPERANDS                 |
 *  +----------------+--------------------------------------+
 * ============================================================ */

/* ---------------- Opcodes (4 bits, 0x0-0xF) ---------------- */
typedef enum {
    OP_BR   = 0x0,  /* conditional branch (checks RCND)        */
    OP_ADD  = 0x1,  /* DR = SR1 + (SR2 | IMM5)                 */
    OP_SUB  = 0x2,  /* DR = SR1 - (SR2 | IMM5)                 */
    OP_MUL  = 0x3,  /* DR = SR1 * (SR2 | IMM5)                 */
    OP_DIV  = 0x4,  /* DR = SR1 / (SR2 | IMM5)                 */
    OP_LD   = 0x5,  /* DR = mem[addr9]    (direct-addressed)   */
    OP_ST   = 0x6,  /* mem[addr9] = SR    (direct-addressed)   */
    OP_LDI  = 0x7,  /* DR = sext(IMM9)    (load constant)      */
    OP_CMP  = 0x8,  /* SR1 - SR2, sets RCND only (no dest)     */
    OP_JMP  = 0x9,  /* RPC = addr12       (unconditional)      */
    OP_RES  = 0xA,  /* reserved                                */
    OP_RES2 = 0xB,  /* reserved                                */
    OP_RES3 = 0xC,  /* reserved                                */
    OP_RES4 = 0xD,  /* reserved                                */
    OP_RES5 = 0xE,  /* reserved                                */
    OP_TRAP = 0xF   /* system call (I/O, halt)                 */
} Opcode;

#define NOPS 16  /* total addressable opcodes (4-bit field) */

/* ---------------- Registers ---------------- */
typedef enum {
    R0 = 0, R1, R2, R3, R4, R5, R6, R7,
    RPC,   /* program counter */
    RCND,  /* condition flags */
    RCNT   /* register count, not a real register */
} Register;

/* ---------------- Condition flags (RCND) ---------------- */
typedef enum {
    FP = 1 << 0,  /* positive */
    FZ = 1 << 1,  /* zero     */
    FN = 1 << 2   /* negative */
} Flag;

/* ---------------- Trap vectors ---------------- */
enum {
    TRAP_READ  = 0x20,  /* read an int into R0   (was tinu16)  */
    TRAP_WRITE = 0x21,  /* print the int in R0   (was toutu16) */
    TRAP_HALT  = 0x25   /* stop execution                       */
};

/* ---------------- Memory layout ---------------- */
#define MEM_SIZE   (UINT16_MAX + 1)  /* 65,536 words */
#define PC_START   0x0000            /* programs load here.

/* ============================================================
 * Bit-field extraction macros
 * ============================================================ */

/* Opcode: bits [15:12] */
#define OPC(i)   ((uint16_t)((i) >> 12))

/* Destination register: bits [11:9] (used by ADD/SUB/MUL/DIV/LD/LDI) */
#define DR(i)    ((uint16_t)(((i) >> 9) & 0x7))

/* Source register 1: bits [8:6] (used by ADD/SUB/MUL/DIV/CMP/ST) */
#define SR1(i)   ((uint16_t)(((i) >> 6) & 0x7))

/* Source register 2: bits [2:0] (register-mode ADD/SUB/MUL/DIV/CMP) */
#define SR2(i)   ((uint16_t)((i) & 0x7))

/* Immediate-mode flag: bit [5] — 1 means "use IMM5 instead of SR2" */
#define FIMM(i)  ((uint16_t)(((i) >> 5) & 0x1))

/* 5-bit immediate: bits [4:0] */
#define IMM5(i)  ((uint16_t)((i) & 0x1F))

/* 9-bit immediate/address: bits [8:0] (LDI operand, BR offset) */
#define IMM9(i)  ((uint16_t)((i) & 0x1FF))

/* 12-bit address: bits [11:0] (JMP direct address, full word range) */
#define ADDR12(i) ((uint16_t)((i) & 0xFFF))

/* 9-bit address: bits [8:0] (LD/ST direct address — DR/SR occupies [11:9]) */
#define ADDR9(i)  ((uint16_t)((i) & 0x1FF))

/* Condition bits for BR: bits [11:9] (N/Z/P), reuses DR's position */
#define BRCOND(i) ((uint16_t)(((i) >> 9) & 0x7))

/* Trap vector: bits [7:0] */
#define TRP(i)   ((uint16_t)((i) & 0xFF))

/* ============================================================
 * Sign extension
 * ============================================================ */
static inline uint16_t sign_ext(uint16_t n, int bits) {
    /* If the top bit of the b-bit field is set, fill the upper
     * bits with 1s to preserve the two's-complement value. */
    return ((n >> (bits - 1)) & 1) ? (uint16_t)(n | (0xFFFF << bits)) : n;
}

/* Convenience: sign-extend the 5-bit or 9-bit immediate fields directly */
#define SEXTIMM5(i)  sign_ext(IMM5(i), 5)
#define SEXTIMM9(i)  sign_ext(IMM9(i), 9)

/* ============================================================
 * Instruction encoding helpers (used by codegen_vm.c)
 * ============================================================
 * Each returns a packed uint16_t ready to write to the .tbc file.
 * These are the inverse of the extraction macros above. */

static inline uint16_t enc_r3(Opcode op, uint16_t dr, uint16_t sr1, uint16_t sr2) {
    /* register-mode arithmetic: OPCODE | DR | SR1 | 0(imm flag) | 00 | SR2 */
    return (uint16_t)((op << 12) | (dr << 9) | (sr1 << 6) | (sr2 & 0x7));
}

static inline uint16_t enc_imm(Opcode op, uint16_t dr, uint16_t sr1, uint16_t imm5) {
    /* immediate-mode arithmetic: OPCODE | DR | SR1 | 1(imm flag) | IMM5 */
    return (uint16_t)((op << 12) | (dr << 9) | (sr1 << 6) | (1 << 5) | (imm5 & 0x1F));
}

static inline uint16_t enc_cmp(uint16_t sr1, uint16_t sr2) {
    return (uint16_t)((OP_CMP << 12) | (sr1 << 6) | (sr2 & 0x7));
}

static inline uint16_t enc_ldi(uint16_t dr, uint16_t imm9) {
    return (uint16_t)((OP_LDI << 12) | (dr << 9) | (imm9 & 0x1FF));
}

static inline uint16_t enc_ld_st(Opcode op, uint16_t reg_, uint16_t addr9) {
    /* op is OP_LD or OP_ST; field at [11:9] is DR for LD, SR for ST.
     * Address is 9 bits (bits [8:0]) since [11:9] is taken by the register —
     * i.e. up to 512 direct-addressable variable slots. */
    return (uint16_t)((op << 12) | (reg_ << 9) | (addr9 & 0x1FF));
}

static inline uint16_t enc_jmp(uint16_t addr12) {
    return (uint16_t)((OP_JMP << 12) | (addr12 & 0xFFF));
}

static inline uint16_t enc_br(uint16_t cond_nzp, uint16_t imm9) {
    return (uint16_t)((OP_BR << 12) | (cond_nzp << 9) | (imm9 & 0x1FF));
}

static inline uint16_t enc_trap(uint16_t trapvect8) {
    return (uint16_t)((OP_TRAP << 12) | (trapvect8 & 0xFF));
}


#endif /* BYTECODE_H */
