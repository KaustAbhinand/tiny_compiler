/*IR -> Bytecode Codegen
 * ============================================================
 * Walks the IRInstruction linked list (ir.h, produced by ir_gen.c,
 * optionally rewritten by optimizer.c) and emits a flat .tbc file
 * of numeric instructions in bytecode.h's format, ready for vm.c.
 *
 * Two passes are required:
 *   Pass 1 — assign every distinct variable a memory slot, and
 *            discover every IR_LABEL's eventual instruction index.
 *   Pass 2 — actually encode each IR instruction, resolving
 *            variable names to slots and label names to addresses
 *            (addresses are already known from pass 1, so no
 *            backpatching is needed).
 *
 * Scratch registers R0/R1 are used to evaluate each instruction;
 * nothing is register-allocated or kept live across instructions,
 * since the IR itself has no notion of register lifetime. This is
 * deliberately simple and always correct, whether or not the IR
 * it's fed has already been optimized upstream.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bytecode.h"
#include "codegen_vm.h"

/* ---------------- symbol table (variable name -> memory slot) ---------------- */

#define MAX_VARS   256   /* well within the 9-bit (512-slot) LD/ST address space */
#define MAX_LABELS 256

typedef struct {
    char name[32];
    uint16_t slot;
} VarEntry;

typedef struct {
    char name[32];
    uint16_t addr; /* instruction index this label refers to */
} LabelEntry;

static VarEntry vars[MAX_VARS];
static int nvars = 0;

static LabelEntry labels[MAX_LABELS];
static int nlabels = 0;

/* Growable output buffer for emitted words */
static uint16_t *code = NULL;
static size_t code_len = 0;
static size_t code_cap = 0;

static void emit_word(uint16_t w) {
    if (code_len == code_cap) {
        code_cap = code_cap ? code_cap * 2 : 256;
        code = realloc(code, code_cap * sizeof(uint16_t));
        if (!code) {
            fprintf(stderr, "codegen_vm: out of memory\n");
            exit(1);
        }
    }
    code[code_len++] = w;
}

/* ---------------- operand classification ---------------- */

/* Returns 1 if s is an integer literal (optionally signed), 0 otherwise. */
static int is_literal(const char *s) {
    if (!s || !*s) return 0;
    const char *p = s;
    if (*p == '+' || *p == '-') p++;
    if (!*p) return 0;
    while (*p) {
        if (!isdigit((unsigned char)*p)) return 0;
        p++;
    }
    return 1;
}

/* ---------------- variable slot table ---------------- */

static uint16_t var_slot(const char *name) {
    for (int i = 0; i < nvars; i++) {
        if (strcmp(vars[i].name, name) == 0) return vars[i].slot;
    }
    if (nvars >= MAX_VARS) {
        fprintf(stderr, "codegen_vm: too many variables (max %d)\n", MAX_VARS);
        exit(1);
    }
    strncpy(vars[nvars].name, name, sizeof(vars[nvars].name) - 1);
    vars[nvars].name[sizeof(vars[nvars].name) - 1] = '\0';
    vars[nvars].slot = (uint16_t)(VAR_START + nvars); /* Offset by the start address (0x0100) */
    return vars[nvars++].slot;
}

/* ---------------- label address table ---------------- */

static void label_define(const char *name, uint16_t addr) {
    if (nlabels >= MAX_LABELS) {
        fprintf(stderr, "codegen_vm: too many labels (max %d)\n", MAX_LABELS);
        exit(1);
    }
    strncpy(labels[nlabels].name, name, sizeof(labels[nlabels].name) - 1);
    labels[nlabels].name[sizeof(labels[nlabels].name) - 1] = '\0';
    labels[nlabels].addr = addr;
    nlabels++;
}

static uint16_t label_addr(const char *name) {
    for (int i = 0; i < nlabels; i++) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].addr;
    }
    fprintf(stderr, "codegen_vm: undefined label '%s'\n", name);
    exit(1);
}

/* ---------------- loading an operand into a register ---------------- */

/* Emits whatever's needed to get `operand`'s value into register `dr`:
 * LDI if it's a literal, LD if it's a variable. */
static void load_operand(const char *operand, uint16_t dr) {
    if (is_literal(operand)) {
        long v = strtol(operand, NULL, 10);
        emit_word(enc_ldi(dr, (uint16_t)v & 0x1FF));
        /* NOTE: LDI's immediate is only 9 bits (sign-extended), so
         * literals outside roughly [-256, 255] will not round-trip
         * correctly. TINY's typical integer literals fit comfortably
         * within this range for coursework-scale programs; widening
         * this (e.g. a wide-load pseudo-op spanning two words) is a
         * natural follow-up if larger constants are ever needed. */
    } else {
        emit_word(enc_ld_st(OP_LD, dr, var_slot(operand)));
    }
}

/* ---------------- relop -> branch condition ---------------- */

/* IR_IF_GOTO's semantics here: jump to label WHEN (arg1 relop arg2) is true.
 * We compute CMP(arg1, arg2) = arg1 - arg2, then branch on the flag that
 * makes "arg1 relop arg2" true. */
static uint16_t relop_to_cond(const char *relop) {
    if (strcmp(relop, "<")  == 0) return FN;          /* arg1-arg2 < 0 */
    if (strcmp(relop, ">")  == 0) return FP;          /* arg1-arg2 > 0 */
    if (strcmp(relop, "==") == 0) return FZ;          /* arg1-arg2 == 0 */
    if (strcmp(relop, "<=") == 0) return (uint16_t)(FN | FZ);
    if (strcmp(relop, ">=") == 0) return (uint16_t)(FP | FZ);
    if (strcmp(relop, "!=") == 0) return (uint16_t)(FN | FP);
    fprintf(stderr, "codegen_vm: unknown relop '%s'\n", relop);
    exit(1);
}

/* ============================================================
 * Pass 1 — assign variable slots, compute each instruction's
 * final address, and record label addresses.
 * ============================================================
 * This requires knowing, for each IR node, how many bytecode
 * words it will expand to — which depends only on its type and
 * whether its operands are literals or variables (an LDI is one
 * word same as an LD, so in this encoding every IR op happens to
 * expand to a fixed word count per type; this function is still
 * written generally in case that stops being true later). */

static int words_for(IRInstruction *ins) {
    switch (ins->type) {
        case IR_ASSIGN: return 2;  /* load arg1 into R0, store R0 to result */
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
            return 4;              /* load arg1->R0, load arg2->R1, op, store */
        case IR_IF_GOTO:
            return 4;              /* load arg1->R0, load arg2->R1, CMP+BR... */
            /* NOTE: actual count below is 3 loads/cmp + 1 br = handled
             * explicitly in emit; see the real count used there. */
        case IR_GOTO:  return 1;
        case IR_LABEL: return 0;   /* labels don't emit code */
        case IR_READ:  return 2;   /* TRAP READ, then ST R0 -> result */
        case IR_WRITE: return 2;   /* load arg1->R0, TRAP WRITE */
        case IR_CMP:   return 8;   /* load arg1, arg2, CMP, BR, LDI 0, JMP, LDI 1, ST-8 words */
        case IR_IF:    return 4;   /* Since ir_gen.c emits IR_IF. */
        default:       return 0;
    }
}

/* Compute the number of words for an instruction */
static int words_for_exact(IRInstruction *ins) {
    return words_for(ins);
}

static void pass1(IRInstruction *ir) {
    uint16_t addr = 0;
    for (IRInstruction *ins = ir; ins != NULL; ins = ins->next) {
        if (ins->type == IR_LABEL) {
            label_define(ins->label, addr);
            continue;
        }
        addr = (uint16_t)(addr + words_for_exact(ins));
    }
}

/* ============================================================
 * Pass 2 — actually emit the bytecode
 * ============================================================ */

static void pass2(IRInstruction *ir) {
    for (IRInstruction *ins = ir; ins != NULL; ins = ins->next) {
        switch (ins->type) {

            case IR_LABEL:
                /* no code; address already recorded in pass 1 */
                break;

            case IR_ASSIGN:
                load_operand(ins->arg1, R0);
                emit_word(enc_ld_st(OP_ST, R0, var_slot(ins->result)));
                break;

            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV: {
                load_operand(ins->arg1, R0);
                load_operand(ins->arg2, R1);
                Opcode op = (ins->type == IR_ADD) ? OP_ADD
                          : (ins->type == IR_SUB) ? OP_SUB
                          : (ins->type == IR_MUL) ? OP_MUL
                          :                         OP_DIV;
                emit_word(enc_r3(op, R0, R0, R1));
                emit_word(enc_ld_st(OP_ST, R0, var_slot(ins->result)));
                break;
            }
	    
	    case IR_IF:
            case IR_IF_GOTO: {
                load_operand(ins->arg1, R0);
                load_operand(ins->arg2, R1);
                emit_word(enc_cmp(R0, R1));
                uint16_t cond = relop_to_cond(ins->relop);
                uint16_t target = label_addr(ins->label);
                /* BR's offset is relative to RPC *after* this instruction
                 * has been fetched (RPC already incremented) — matches
                 * vm.c's br(), which adds the offset post-increment. */
                uint16_t this_instr_addr = (uint16_t)(code_len + 1);
                int16_t offset = (int16_t)(target - this_instr_addr);
                emit_word(enc_br(cond, (uint16_t)offset & 0x1FF));
                break;
            }

            case IR_GOTO: {
                /* JMP uses an absolute 12-bit address, not an offset,
                 * so no RPC-relative arithmetic is needed here. */
                emit_word(enc_jmp(label_addr(ins->label)));
                break;
            }

            case IR_READ:
                emit_word(enc_trap(TRAP_READ));
                emit_word(enc_ld_st(OP_ST, R0, var_slot(ins->result)));
                break;

            case IR_WRITE:
                load_operand(ins->arg1, R0);
                emit_word(enc_trap(TRAP_WRITE));
                break;

            case IR_CMP: 
		{
		 load_operand(ins->arg1, R0);
	         load_operand(ins->arg2, R1);
		 emit_word(enc_cmp(R0, R1));
		 uint16_t cond = relop_to_cond(ins->relop);
		 uint16_t br_addr = (uint16_t)(code_len + 1);
                emit_word(enc_br(cond, (uint16_t)(2) & 0x1FF));
 
                /* condition false: result = 0, then jump past the true-case */
                emit_word(enc_ldi(R0, 0));
                uint16_t jmp_over_addr = (uint16_t)(code_len);
                emit_word(0); /* placeholder JMP, patched below */
 
                /* condition true: result = 1 */
                uint16_t true_addr = (uint16_t)code_len;
                emit_word(enc_ldi(R0, 1));


                /* patch the JMP that skips the true-case to land here.
                 * Same PC_START offset as IR_GOTO above — JMP targets are
                 * absolute addresses in the VM's real address space, not
                 * word indices from the start of the program. */
                uint16_t after_addr = (uint16_t)code_len;
                code[jmp_over_addr] = enc_jmp((uint16_t)(PC_START + after_addr));
                (void)br_addr; (void)true_addr;
 
                emit_word(enc_ld_st(OP_ST, R0, var_slot(ins->result)));
                break;
           };

            default:
                fprintf(stderr, "codegen_vm: unhandled IR type %d\n", ins->type);
                exit(1);
        }
    }
}

/* ============================================================
 * Public entry point
 * ============================================================ */

int generate_bytecode(IRInstruction *ir, const char *out_path) {
    nvars = 0;
    nlabels = 0;
    code_len = 0;

    pass1(ir);
    pass2(ir);
    emit_word(enc_trap(TRAP_HALT));

    FILE *f = fopen(out_path, "wb");
    if (!f) {
        perror("codegen_vm: fopen");
        return -1;
    }
    size_t written = fwrite(code, sizeof(uint16_t), code_len, f);
    fclose(f);

    if (written != code_len) {
        fprintf(stderr, "codegen_vm: short write to %s\n", out_path);
        return -1;
    }

    return 0;
}
