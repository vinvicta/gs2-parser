#pragma once

#ifndef GS2DECOMPILER_H
#define GS2DECOMPILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "opcodes.h"
#include "encoding/buffer.h"

namespace gs2decompiler {

// Represents a single function with its bytecode range
struct FunctionInfo {
    std::string name;
    uint32_t opIndex;
    uint32_t endOpIndex;
    std::vector<uint8_t> bytecode;
};

// Represents a decoded instruction
struct Instruction {
    opcode::Opcode op;
    size_t bytecodeOffset;
    int32_t operandInt;
    double operandDouble;
    std::string operandString;

    Instruction() : op(opcode::OP_NONE), bytecodeOffset(0), operandInt(0), operandDouble(0.0) {}
};

// Stack simulation for operands
struct StackOperand {
    enum class Type {
        NUMBER,
        STRING,
        VAR,
        ARRAY,
        OBJECT,
        BOOLEAN,
        NULL_VAL,
        UNKNOWN
    };

    Type type;
    std::string value;
    bool isLValue;  // Can be assigned to

    StackOperand() : type(Type::UNKNOWN), isLValue(false) {}
    StackOperand(Type t, const std::string& v, bool lval = false)
        : type(t), value(v), isLValue(lval) {}
};

// Control flow information
struct ControlFlowInfo {
    enum class CFType {
        IF,
        ELSE,
        WHILE,
        FOR,
        FOREACH,
        SWITCH,
        WITH
    };

    CFType type;
    uint32_t startOpIndex;
    uint32_t endOpIndex;
    int indentLevel;

    ControlFlowInfo() : type(CFType::IF), startOpIndex(0), endOpIndex(0), indentLevel(0) {}
};

class GS2Decompiler {
public:
    GS2Decompiler();
    ~GS2Decompiler();

    // Load bytecode from file
    bool loadBytecode(const std::string& filename);

    // Decompile to string
    std::string decompile();

    // Get error message if something went wrong
    std::string getError() const { return error; }

private:
    // Segment parsing
    bool parseSegments();
    bool parseGS1Flags(size_t offset, size_t length);
    bool parseFunctionTable(size_t offset, size_t length);
    bool parseStringTable(size_t offset, size_t length);
    bool parseBytecodeSegment(size_t offset, size_t length);

    // Dynamic number reading
    bool readDynamicNumber(size_t& pos, int32_t& outVal);
    bool readDynamicNumberUnsigned(size_t& pos, uint32_t& outVal);
    bool readDoubleNumber(size_t& pos, std::string& outVal);

    // Instruction decoding
    bool decodeInstructions();
    Instruction* getNextInstruction(size_t& pos);
    size_t getInstructionSize(opcode::Opcode op, size_t pos);

    // Source code generation
    std::string generateSource();
    std::string generateFunction(const FunctionInfo& func);
    std::string generateStatement(uint32_t& opIndex, int indent, const FunctionInfo& func);
    std::string generateExpression(uint32_t& opIndex, const FunctionInfo& func);

    // Helper methods for expression generation
    std::string generateBinaryOpExpression(uint32_t& opIndex, const FunctionInfo& func,
                                          const std::string& opStr);
    std::string generateUnaryOpExpression(uint32_t& opIndex, const FunctionInfo& func,
                                          const std::string& opStr, bool prefix);
    std::string generateFunctionCall(uint32_t& opIndex, const FunctionInfo& func);
    std::string generateMemberAccess(uint32_t& opIndex, const FunctionInfo& func);

    // Control flow reconstruction
    bool analyzeControlFlow(const FunctionInfo& func);
    bool isJumpTarget(uint32_t opIndex) const;

    // Utility
    std::string getStringFromTable(int32_t index);
    std::string indentString(int level);
    void setError(const std::string& err) { error = err; }

private:
    // Raw bytecode data
    std::vector<uint8_t> bytecodeData;

    // Parsed segments
    std::vector<std::string> stringTable;
    std::vector<FunctionInfo> functions;

    // Decoded instructions per function
    std::unordered_map<std::string, std::vector<Instruction>> functionInstructions;

    // Control flow analysis data
    std::unordered_map<uint32_t, bool> jumpTargets;

    // State
    std::string error;
};

} // namespace gs2decompiler

#endif // GS2DECOMPILER_H
