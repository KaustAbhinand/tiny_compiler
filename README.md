This is a compiler for the language: TINY, a subset of C.
Documenation:

Documentation of TINY:
Every line is to be terminated with a semicolon. (;).
The program needs to be terminated with the keyword "end". A semicolon must not be placed after this keyword.
Variables are declared by assigning values to them directly. They are block scoped. Each block is defined by an indentation, preferably using tab space for each block.

Syntax:
input: read
output: write
if statements: if <statement> then <code> end
if-else statements: if <statement> then <code> else <code> end
loop statement:     repeat <code> until <condition> end
No boolean operators.
Only integer variables.
No arrays, or functions.


The program outputs TARGET INDEPENDENT assembly code. i.e. it doesn't pertain to any ISA. ISA specific code, will be added very soon.

Stack: (congrats if you actually read this far lol)
scanner - built using lex.
parser - built using YACC.
AST, IR, codegen:  C. (custom code)

The 
