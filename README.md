This is a compiler for the language: TINY, a subset of C.

## Documentation
- Every line is to be terminated with a semicolon. (;).
- The program needs to be terminated with the keyword "end".
- The "end" keyword must not be followed by a semicolon.
- Variables are declared by assigning values to them directly. They are block scoped. Each block is defined by an indentation, preferably using tab space for each block.

## Syntax
**input**: `read`  
**output**: `write`  
**if statements**: `if <statement> then <code> end`  
**if-else statements**: `if <statement> then <code> else <code> end`  
**loop statement**: `repeat <code> until <condition> end`  
**Comments**: `{ enclosed in these }`

**NO boolean operators**.  
Only integer variables.  
No arrays, or functions.


Stack: (congrats if you actually read this far lol)

- scanner: built using lex.
- parser: built using YACC.
- AST, IR, Codegen: C. (custom code)
- VM: C

## Build steps:
- Fork and clone the repo
- chmod +x build.sh
- chmod +x tinyshell.sh
- ./tinyshell.sh
- Just enter the file name/path. The program stays in the loop until CTRL+C.
