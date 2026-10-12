#!/bin/sh
set -e

#flex scanner.l
#bison -d parser.y

gcc -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 \
    parser.tab.c ir_gen.c ast.c codegen.c codegen_vm.c lex.yy.c main.c -o tiny

gcc -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 vm.c -o tinyvm

echo "Built: tiny, tinyvm"
