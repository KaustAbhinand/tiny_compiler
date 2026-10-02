 /*von Neumann, LC-3-style VM: 65,536 words of memory, a small
 * register file, 4-bit-opcode instructions, fetch-decode-execute
 * loop with function-pointer dispatch.
 *
 * Modeled directly on:
 * https://www.andreinc.net/2021/12/01/writing-a-simple-vm-in-less-than-125-lines-of-c/
 *
 * This file only ever sees bytecode.h's instruction format. It
 * knows nothing about TINY source, the AST, or the IR — it is a
 * dumb, general executor for whatever codegen_vm.c produced.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bytecode.h"
#include "vm.h"

uint16_t mem[MEM_SIZE];
uint16_t reg[RCNT];

static int running = 1;

/* ---------------- memory access helpers ---------------- */
static inline uint16_t mr(uint16_t address) { return mem[address]; }
static inline void mw(uint16_t address, uint16_t val) { mem[address] = val; }

/* ---------------- condition flag update ---------------- */
/* Call after any instruction that writes a register, exactly like
 * the article's uf(). Sets RCND based on the signed value now in reg[r]. */
static void uf(uint16_t r) {
    if (reg[r] == 0) {
        reg[RCND] = FZ;
    } else if ((int16_t)reg[r] < 0) {
        reg[RCND] = FN;
    } else {
        reg[RCND] = FP;
    }
}

/* ============================================================
 * Opcode handlers
 * Each takes the raw 16-bit instruction and does its job.
 * ============================================================ */

static void br(uint16_t i) {
    uint16_t cond = BRCOND(i);
    if (cond & reg[RCND]) {
        reg[RPC] = (uint16_t)(reg[RPC] + (int16_t)SEXTIMM9(i));
    }
}

static void add(uint16_t i) {
    uint16_t dr = DR(i), sr1 = SR1(i);
    uint16_t b = FIMM(i) ? SEXTIMM5(i) : reg[SR2(i)];
    reg[dr] = (uint16_t)(reg[sr1] + b);
    uf(dr);
}

static void sub(uint16_t i) {
    uint16_t dr = DR(i), sr1 = SR1(i);
    uint16_t b = FIMM(i) ? SEXTIMM5(i) : reg[SR2(i)];
    reg[dr] = (uint16_t)(reg[sr1] - b);
    uf(dr);
}

static void mul(uint16_t i) {
    uint16_t dr = DR(i), sr1 = SR1(i);
    uint16_t b = FIMM(i) ? SEXTIMM5(i) : reg[SR2(i)];
    reg[dr] = (uint16_t)((int16_t)reg[sr1] * (int16_t)b);
    uf(dr);
}

static void div_(uint16_t i) {
    uint16_t dr = DR(i), sr1 = SR1(i);
    uint16_t b = FIMM(i) ? SEXTIMM5(i) : reg[SR2(i)];
    if (b == 0) {
        fprintf(stderr, "vm: division by zero at pc=0x%04x\n", (unsigned)(reg[RPC] - 1));
        exit(1);
    }
    reg[dr] = (uint16_t)((int16_t)reg[sr1] / (int16_t)b);
    uf(dr);
}

static void ld(uint16_t i) {
    uint16_t dr = DR(i);
    reg[dr] = mr(ADDR9(i));
    uf(dr);
}

static void st(uint16_t i) {
    uint16_t sr = DR(i); /* same bit position as DR, named differently for clarity */
    mw(ADDR9(i), reg[sr]);
}

static void ldi(uint16_t i) {
    uint16_t dr = DR(i);
    reg[dr] = SEXTIMM9(i);
    uf(dr);
}

static void cmp(uint16_t i) {
    uint16_t a = reg[SR1(i)];
    uint16_t b = reg[SR2(i)];
    uint16_t result = (uint16_t)((int16_t)a - (int16_t)b);
    if (result == 0)      reg[RCND] = FZ;
    else if ((int16_t)result < 0) reg[RCND] = FN;
    else  reg[RCND] = FP;
}

static void jmp(uint16_t i) {
    reg[RPC] = ADDR12(i);
}

/* ---------------- TRAP / I/O ---------------- */
static void tread(void) {
    long v;
    if (scanf("%ld", &v) != 1) {
        fprintf(stderr, "vm: failed to read input\n");
        exit(1);
    }
    reg[R0] = (uint16_t)v;
}

static void twrite(void) {
    printf("%d\n", (int16_t)reg[R0]);
    fflush(stdout);
}

static void thalt(void) {
    running = 0;
}

static void trap(uint16_t i) {
    switch (TRP(i)) {
        case TRAP_READ:  tread();  break;
        case TRAP_WRITE: twrite(); break;
        case TRAP_HALT:  thalt();  break;
        default:
            fprintf(stderr, "vm: unknown trap vector 0x%02x at pc=0x%04x\n",
                    TRP(i), (unsigned)(reg[RPC] - 1));
            exit(1);
    }
}

static void bad_opcode(uint16_t i) {
    fprintf(stderr, "vm: illegal opcode 0x%x at pc=0x%04x\n",
            OPC(i), (unsigned)(reg[RPC] - 1));
    exit(1);
}

/* ---------------- dispatch table ---------------- */
typedef void (*op_fn)(uint16_t);

static op_fn op_ex[NOPS] = {
    /* 0x0 */ br,
    /* 0x1 */ add,
    /* 0x2 */ sub,
    /* 0x3 */ mul,
    /* 0x4 */ div_,
    /* 0x5 */ ld,
    /* 0x6 */ st,
    /* 0x7 */ ldi,
    /* 0x8 */ cmp,
    /* 0x9 */ jmp,
    /* 0xA */ bad_opcode,
    /* 0xB */ bad_opcode,
    /* 0xC */ bad_opcode,
    /* 0xD */ bad_opcode,
    /* 0xE */ bad_opcode,
    /* 0xF */ trap
};

/* ============================================================
 * Image loading
 * ============================================================
 * .tbc format: a flat sequence of big-endian uint16_t words,
 * loaded verbatim into mem[] starting at PC_START. No header —
 * codegen_vm.c always emits code starting at PC_START, so the
 * VM doesn't need an origin field (kept deliberately simple;
 * can be added later without breaking this loader).
 * ============================================================ */
int ld_img(const char *fname) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        perror("vm: fopen");
        return -1;
    }

    uint16_t *dest = mem + PC_START;
    size_t max_words = MEM_SIZE - PC_START;
    size_t n = fread(dest, sizeof(uint16_t), max_words, f);
    fclose(f);

    if (n == 0) {
        fprintf(stderr, "vm: empty or unreadable image: %s\n", fname);
        return -1;
    }

    /* .tbc files are written in the host's native byte order by
     * codegen_vm.c, so no byte-swapping is needed here as long as
     * the same machine (or architecture family) compiles and runs.
     * If cross-platform portability is ever needed, swap to a
     * fixed little-endian format on both ends. */

    return (int)n;
}

/* ============================================================
 * The fetch-decode-execute loop
 * ============================================================ */
void run(void) {
    reg[RPC] = PC_START;
    running = 1;

    while (running) {
        uint16_t instr = mem[reg[RPC]++];
        uint16_t op = OPC(instr);
        op_ex[op](instr);
    }
}

/* ============================================================
 * Standalone entry point: tinyvm program.tbc (NOT the actual language
 * entry point.)
 * ============================================================ */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <program.tbc>\n", argv[0]);
        return 1;
    }

    memset(mem, 0, sizeof(mem));
    memset(reg, 0, sizeof(reg));

    if (ld_img(argv[1]) < 0) {
        return 1;
    }

    run();
    return 0;
}


