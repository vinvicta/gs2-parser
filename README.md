# gs2-parser

A **compiler and decompiler** for the Graal Script 2 (GS2) language used in GraalOnline games.

## Features

- **GS2 Compiler**: Convert GS2 source code (`.gs2`, `.txt`) to bytecode (`.gs2bc`)
- **GS2 Decompiler**: Decompile bytecode files back to human-readable disassembly
- **Full AST-based compilation**: Proper parsing and semantic analysis
- **Multi-file support**: Process multiple files or directories at once
- **WASM support**: Compile to WebAssembly for browser usage

## Prerequisites

Before building, clone the repository and recursively clone the submodules:

```sh
git clone https://github.com/vinvicta/gs2-parser.git --recursive
cd gs2-parser
```

### Dependencies

- **CMake** (3.10+)
- **C++ Compiler** (C++17 support required: GCC 7+, Clang 5+, MSVC 2017+)
- **Bison** (3.4+)
- **Flex** (2.6+)

#### Ubuntu/Debian
```sh
sudo apt-get install cmake g++ bison flex
```

#### macOS
```sh
brew install cmake bison flex
```

#### Windows (with vcpkg)
```cmd
vcpkg install cmake bison flex
```

## Building

### Native Build

```sh
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The executable will be created at `../bin/gs2test`.

### WebAssembly Build

First, ensure you have [Emscripten](https://emscripten.org/) installed. Then:

```sh
mkdir build
cd build
emcmake cmake ..
make -j$(nproc)
```

The resulting `gs2test.js` and `gs2test.wasm` files can be imported into a webpage.

## Usage

### Compiler

**Basic usage:**
```sh
./bin/gs2test script.gs2
# Creates: script.gs2bc
```

**With custom output:**
```sh
./bin/gs2test script.gs2 -o output.gs2bc
```

**Verbose mode:**
```sh
./bin/gs2test script.gs2 -v
```

### Decompiler

The decompiler converts `.gs2bc` bytecode files back to human-readable format.

**Basic decompilation:**
```sh
./bin/gs2test script.gs2bc -d
# Creates: script.gs2
```

**With custom output:**
```sh
./bin/gs2test script.gs2bc -d -o decompiled.gs2
```

**Verbose decompilation:**
```sh
./bin/gs2test script.gs2bc -d -v
```

### Command-Line Options

```
GS2 Script Compiler/Decompiler

Usage:
  gs2test [OPTIONS] INPUT [OUTPUT]
  gs2test INPUT -o OUTPUT
  gs2test --help

Arguments:
  INPUT              Input file (.gs2, .txt, or .gs2bc) or directory
  OUTPUT             Output file (.gs2bc for compile, .gs2 for decompile)

Options:
  -o, --output FILE  Specify output file
  -d, --decompile    Decompile .gs2bc to .gs2 source
  -v, --verbose      Verbose output
  -h, --help         Show this help message

Examples:
  gs2test script.gs2                    # Creates script.gs2bc (compile)
  gs2test script.gs2bc -d               # Creates script.gs2 (decompile)
  gs2test script.gs2 output.gs2bc       # Creates output.gs2bc
  gs2test script.gs2bc -o output.gs2 -d # Creates output.gs2 (decompile)
  gs2test scripts/                      # Process directory
  gs2test file1.gs2 file2.gs2 file3.gs2 # Process multiple files
```

### Multi-File and Directory Processing

**Process a directory:**
```sh
./bin/gs2test scripts/
```

**Process multiple files:**
```sh
./bin/gs2test file1.gs2 file2.gs2 file3.gs2
```

## Decompiler Output

The decompiler generates a disassembly showing:

- Function structure and names
- String constants from the string table
- Opcode names and descriptions
- Operands (numbers, strings, jump offsets)

**Example output:**
```gs2
function onCreated() {
  // Decompilation not fully implemented yet
  // Function: onCreated
  // Opcodes: 52 bytes
  // OP_SET_INDEX
  // OP_TYPE_ARRAY
  // OP_FUNC_PARAMS_END
  // OP_JMP 2493
  // OP_TYPE_VAR "x"
  // OP_MEMBER_ACCESS
  // OP_TYPE_NUMBER 5
  // OP_ASSIGN
  // OP_TEMP
  // OP_TYPE_VAR "y"
  // OP_MEMBER_ACCESS
  // OP_TYPE_NUMBER 10
  // OP_ASSIGN
  // OP_TYPE_ARRAY
  // OP_TYPE_STRING "Result: "
  // OP_TEMP
  // OP_TYPE_VAR "x"
  // OP_MEMBER_ACCESS
  // OP_CONV_TO_FLOAT
  // OP_TEMP
  // OP_TYPE_VAR "y"
  // OP_MEMBER_ACCESS
  // OP_CONV_TO_FLOAT
  // OP_ADD
  // OP_CONV_TO_STRING
  // OP_JOIN
  // OP_TYPE_VAR "echo"
  // OP_CALL
  // OP_INDEX_DEC
  // OP_TRUE
  // OP_RET
}
```

## Bytecode Format

GS2 bytecode files (`.gs2bc`) consist of 4 segments:

1. **GS1 Flags Segment** (type 1)
   - Bitflags for GS1 events

2. **Function Table Segment** (type 2)
   - List of functions with their opcodes and names
   - Format: `[opIndex: 4 bytes][functionName: null-terminated string]`

3. **String Table Segment** (type 3)
   - All string constants used in the script
   - Format: `[string: null-terminated]`

4. **Bytecode Segment** (type 4)
   - The actual bytecode instructions
   - Format: `[opcode: 1 byte][operands: variable]`

### Dynamic Number Encoding

Numbers are encoded with a prefix byte:

- `0xF0-F2`: Signed integers (1, 2, 4 bytes)
- `0xF3-F5`: Numbers after `OP_TYPE_NUMBER` (1, 2, 4 bytes)
- `0xF6`: Double-precision float (as null-terminated string)

## Architecture

The compiler uses a multi-stage pipeline:

1. **Lexical Analysis**: Flex-based scanner tokenizes GS2 source
2. **Parsing**: Bison-based parser builds Abstract Syntax Tree (AST)
3. **Semantic Analysis**: Type checking and symbol resolution
4. **Code Generation**: Visitor pattern emits bytecode
5. **Optimization**: Jump label resolution and optimization

### Key Components

- **`src/parser.y`**: Bison grammar for GS2 language
- **`src/scanner.l`**: Flex lexer for tokenization
- **`src/ast/`**: AST node definitions
- **`src/visitors/GS2CompilerVisitor.cpp`**: Bytecode emission
- **`src/visitors/GS2Decompiler.cpp`**: Bytecode parsing and disassembly
- **`src/GS2Bytecode.cpp`**: Bytecode buffer management
- **`src/opcodes.h`**: Opcode definitions and mappings

## Testing

Run the test suite:

```sh
cd build
make run-tests
```

Generate test baselines:

```sh
make test-baselines
```

Clean test artifacts:

```sh
make test-clean
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

- Original compiler by **xtjoeytx**
- Decompiler implementation by **vinvicta**
- Based on the GraalOnline GS2 language specification
