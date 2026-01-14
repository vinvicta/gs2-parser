GS2 Script Compiler/Disassembler v1.5.0
==========================================

This is the GS2 Script Compiler/Disassembler for GraalOnline GScript 2 language.

QUICK START
-----------

Compile GS2 source to bytecode:
  ./gs2test script.gs2

Disassemble bytecode to human-readable format:
  ./gs2test script.gs2bc -d

For more options:
  ./gs2test --help

FEATURES
--------
- Compile GS2 source code (.gs2, .txt) to bytecode (.gs2bc)
- Disassemble bytecode files to human-readable format
- Multi-file and directory processing support
- Full AST-based compilation
- Supports all GS2 opcodes and language features

REQUIREMENTS
------------
- Linux x86_64
- No external dependencies required

DOCUMENTATION
-------------
Full documentation available at:
https://github.com/vinvicta/gs2-parser

LICENSE
-------
MIT License - See LICENSE file in source repository

CREDITS
-------
- Original compiler: xtjoeytx
- Disassembler implementation: vinvicta
