#include "GS2Decompiler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iomanip>

namespace gs2decompiler {

enum {
    SEGMENT_GS1FLAGS = 1,
    SEGMENT_FUNCTIONTABLE = 2,
    SEGMENT_STRINGTABLE = 3,
    SEGMENT_BYTECODE = 4
};

GS2Decompiler::GS2Decompiler() {
}

GS2Decompiler::~GS2Decompiler() {
}

bool GS2Decompiler::loadBytecode(const std::string& filename) {
    // Clear previous state
    bytecodeData.clear();
    stringTable.clear();
    functions.clear();
    functionInstructions.clear();
    jumpTargets.clear();
    error.clear();

    // Read file
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        setError("Cannot open file: " + filename);
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    bytecodeData.resize(fileSize);
    file.read(reinterpret_cast<char*>(bytecodeData.data()), fileSize);

    if (!file) {
        setError("Failed to read file: " + filename);
        return false;
    }

    // Parse segments
    if (!parseSegments()) {
        return false;
    }

    // Decode instructions for each function
    return decodeInstructions();
}

bool GS2Decompiler::parseSegments() {
    size_t pos = 0;

    while (pos < bytecodeData.size()) {
        // Need at least 8 bytes for segment header
        if (pos + 8 > bytecodeData.size()) {
            // End of file reached
            break;
        }

        // Read segment type and length (big-endian 32-bit integers)
        uint32_t segType = (static_cast<uint32_t>(bytecodeData[pos]) << 24) |
                          (static_cast<uint32_t>(bytecodeData[pos + 1]) << 16) |
                          (static_cast<uint32_t>(bytecodeData[pos + 2]) << 8) |
                          static_cast<uint32_t>(bytecodeData[pos + 3]);

        uint32_t segLength = (static_cast<uint32_t>(bytecodeData[pos + 4]) << 24) |
                            (static_cast<uint32_t>(bytecodeData[pos + 5]) << 16) |
                            (static_cast<uint32_t>(bytecodeData[pos + 6]) << 8) |
                            static_cast<uint32_t>(bytecodeData[pos + 7]);

        pos += 8;

        // Validate segment type
        if (segType < 1 || segType > 4) {
            setError("Invalid segment type: " + std::to_string(segType));
            return false;
        }

        if (pos + segLength > bytecodeData.size()) {
            setError("Segment extends beyond file bounds");
            return false;
        }

        switch (segType) {
            case SEGMENT_GS1FLAGS:
                if (!parseGS1Flags(pos, segLength))
                    return false;
                break;
            case SEGMENT_FUNCTIONTABLE:
                if (!parseFunctionTable(pos, segLength))
                    return false;
                break;
            case SEGMENT_STRINGTABLE:
                if (!parseStringTable(pos, segLength))
                    return false;
                break;
            case SEGMENT_BYTECODE:
                if (!parseBytecodeSegment(pos, segLength))
                    return false;
                break;
            default:
                break;
        }

        pos += segLength;
    }

    return true;
}

bool GS2Decompiler::parseGS1Flags(size_t offset, size_t length) {
    // GS1 flags - not needed for decompilation
    return true;
}

bool GS2Decompiler::parseFunctionTable(size_t offset, size_t length) {
    size_t pos = offset;

    while (pos < offset + length) {
        if (pos + 4 > bytecodeData.size()) {
            setError("Invalid function table entry");
            return false;
        }

        // Read function opIndex (big-endian)
        uint32_t opIndex = (static_cast<uint32_t>(bytecodeData[pos]) << 24) |
                          (static_cast<uint32_t>(bytecodeData[pos + 1]) << 16) |
                          (static_cast<uint32_t>(bytecodeData[pos + 2]) << 8) |
                          static_cast<uint32_t>(bytecodeData[pos + 3]);
        pos += 4;

        // Read null-terminated function name
        std::string funcName;
        while (pos < bytecodeData.size() && bytecodeData[pos] != 0) {
            funcName += static_cast<char>(bytecodeData[pos]);
            pos++;
        }
        if (pos < bytecodeData.size()) {
            pos++; // Skip null terminator
        }

        FunctionInfo info;
        info.name = funcName;
        info.opIndex = opIndex;
        info.endOpIndex = 0; // Will be determined later
        functions.push_back(info);
    }

    // Sort functions by opIndex
    std::sort(functions.begin(), functions.end(),
              [](const FunctionInfo& a, const FunctionInfo& b) {
                  return a.opIndex < b.opIndex;
              });

    // Set end indices
    for (size_t i = 0; i < functions.size(); i++) {
        if (i + 1 < functions.size()) {
            functions[i].endOpIndex = functions[i + 1].opIndex;
        } else {
            functions[i].endOpIndex = UINT32_MAX; // Last function
        }
    }

    return true;
}

bool GS2Decompiler::parseStringTable(size_t offset, size_t length) {
    size_t pos = offset;

    while (pos < offset + length) {
        std::string str;
        while (pos < bytecodeData.size() && bytecodeData[pos] != 0) {
            str += static_cast<char>(bytecodeData[pos]);
            pos++;
        }
        if (pos < bytecodeData.size()) {
            pos++; // Skip null terminator
        }
        stringTable.push_back(str);
    }

    return true;
}

bool GS2Decompiler::parseBytecodeSegment(size_t offset, size_t length) {
    // Store bytecode references for each function
    for (auto& func : functions) {
        // Calculate actual bytecode offset for this function
        // The opIndex is relative to the bytecode segment
        size_t funcStart = offset + func.opIndex;
        size_t funcEnd = (func.endOpIndex == UINT32_MAX) ?
                        (offset + length) : (offset + func.endOpIndex);

        // Bounds check
        if (funcStart > bytecodeData.size()) {
            funcStart = bytecodeData.size();
        }
        if (funcEnd > bytecodeData.size()) {
            funcEnd = bytecodeData.size();
        }

        // Extract bytecode for this function
        if (funcStart <= funcEnd && funcStart <= bytecodeData.size()) {
            func.bytecode.assign(bytecodeData.begin() + funcStart,
                                bytecodeData.begin() + funcEnd);
        }
    }

    return true;
}

bool GS2Decompiler::readDynamicNumber(size_t& pos, int32_t& outVal) {
    // This is for reading from function bytecode, not the main bytecodeData
    // We'll implement this per-function in decodeInstructions
    return false;
}

bool GS2Decompiler::readDynamicNumberUnsigned(size_t& pos, uint32_t& outVal) {
    return false;
}

bool GS2Decompiler::readDoubleNumber(size_t& pos, std::string& outVal) {
    return false;
}

bool GS2Decompiler::decodeInstructions() {
    // For each function, decode its bytecode into instructions
    for (const auto& func : functions) {
        std::vector<Instruction> instructions;
        const auto& funcBytecode = func.bytecode;
        size_t pos = 0;

        while (pos < funcBytecode.size()) {
            Instruction inst;
            inst.bytecodeOffset = pos;

            if (pos >= funcBytecode.size()) {
                break;
            }

            inst.op = static_cast<opcode::Opcode>(funcBytecode[pos]);
            pos++;

            // Read operands based on opcode
            switch (inst.op) {
                case opcode::OP_SET_INDEX:
                case opcode::OP_SET_INDEX_TRUE:
                case opcode::OP_JMP:
                case opcode::OP_IF:
                case opcode::OP_AND:
                case opcode::OP_OR:
                case opcode::OP_WITH:
                case opcode::OP_FOREACH:
                case opcode::OP_WITHEND:
                    // These have a jump offset operand (big-endian 16-bit)
                    if (pos + 2 <= funcBytecode.size()) {
                        uint16_t jumpOffset = (static_cast<uint16_t>(funcBytecode[pos]) << 8) |
                                              static_cast<uint16_t>(funcBytecode[pos + 1]);
                        inst.operandInt = static_cast<int16_t>(jumpOffset);
                        pos += 2;

                        // Calculate target opIndex for jump tracking
                        if (inst.op == opcode::OP_JMP || inst.op == opcode::OP_IF ||
                            inst.op == opcode::OP_AND || inst.op == opcode::OP_OR ||
                            inst.op == opcode::OP_SET_INDEX || inst.op == opcode::OP_SET_INDEX_TRUE) {
                            uint32_t currentOpIndex = instructions.size();
                            uint32_t targetOpIndex = currentOpIndex + inst.operandInt;
                            jumpTargets[targetOpIndex] = true;
                        }
                    }
                    break;

                case opcode::OP_TYPE_NUMBER: {
                    // Followed by a dynamic number
                    if (pos >= funcBytecode.size()) break;

                    uint8_t numPrefix = funcBytecode[pos];
                    pos++;

                    if (numPrefix == 0xF6) {
                        // Double number as string
                        std::string doubleStr;
                        while (pos < funcBytecode.size() && funcBytecode[pos] != 0) {
                            doubleStr += static_cast<char>(funcBytecode[pos]);
                            pos++;
                        }
                        if (pos < funcBytecode.size()) pos++; // Skip null
                        inst.operandString = doubleStr;
                    } else if (numPrefix >= 0xF0 && numPrefix <= 0xF5) {
                        // Integer
                        bool isSigned = (numPrefix >= 0xF3);
                        int byteCount = 1 << (numPrefix % 3);

                        if (pos + byteCount <= funcBytecode.size()) {
                            if (byteCount == 1) {
                                int8_t val = static_cast<int8_t>(funcBytecode[pos]);
                                inst.operandInt = val;
                            } else if (byteCount == 2) {
                                int16_t val = (static_cast<int16_t>(funcBytecode[pos]) << 8) |
                                             static_cast<int16_t>(funcBytecode[pos + 1]);
                                inst.operandInt = val;
                            } else if (byteCount == 4) {
                                int32_t val = (static_cast<int32_t>(funcBytecode[pos]) << 24) |
                                             (static_cast<int32_t>(funcBytecode[pos + 1]) << 16) |
                                             (static_cast<int32_t>(funcBytecode[pos + 2]) << 8) |
                                             static_cast<int32_t>(funcBytecode[pos + 3]);
                                inst.operandInt = val;
                            }
                            pos += byteCount;
                        }
                    }
                    break;
                }

                case opcode::OP_TYPE_STRING:
                case opcode::OP_TYPE_VAR: {
                    // Followed by an unsigned dynamic number (string table index)
                    if (pos >= funcBytecode.size()) break;

                    uint8_t numPrefix = funcBytecode[pos];
                    pos++;

                    if (numPrefix >= 0xF0 && numPrefix <= 0xF2) {
                        int byteCount = 1 << (numPrefix - 0xF0);
                        uint32_t strIdx = 0;

                        if (pos + byteCount <= funcBytecode.size()) {
                            if (byteCount == 1) {
                                strIdx = static_cast<uint8_t>(funcBytecode[pos]);
                            } else if (byteCount == 2) {
                                strIdx = (static_cast<uint16_t>(funcBytecode[pos]) << 8) |
                                        static_cast<uint16_t>(funcBytecode[pos + 1]);
                            } else if (byteCount == 4) {
                                strIdx = (static_cast<uint32_t>(funcBytecode[pos]) << 24) |
                                        (static_cast<uint32_t>(funcBytecode[pos + 1]) << 16) |
                                        (static_cast<uint32_t>(funcBytecode[pos + 2]) << 8) |
                                        static_cast<uint32_t>(funcBytecode[pos + 3]);
                            }
                            pos += byteCount;

                            inst.operandInt = strIdx;
                            if (strIdx < stringTable.size()) {
                                inst.operandString = stringTable[strIdx];
                            }
                        }
                    }
                    break;
                }

                default:
                    // Most opcodes have no immediate operands
                    break;
            }

            instructions.push_back(inst);
        }

        functionInstructions[func.name] = instructions;
    }

    return true;
}

std::string GS2Decompiler::decompile() {
    return generateSource();
}

std::string GS2Decompiler::generateSource() {
    std::stringstream output;

    for (const auto& func : functions) {
        output << generateFunction(func) << "\n\n";
    }

    return output.str();
}

std::string GS2Decompiler::generateFunction(const FunctionInfo& func) {
    std::stringstream output;

    // Parse function name for public/object qualifiers
    std::string funcName = func.name;
    bool isPublic = false;
    bool hasObject = false;
    std::string objectName;

    size_t dotPos = funcName.find('.');
    if (dotPos != std::string::npos) {
        std::string firstPart = funcName.substr(0, dotPos);
        if (firstPart == "public") {
            isPublic = true;
            size_t secondDot = funcName.find('.', dotPos + 1);
            if (secondDot != std::string::npos) {
                objectName = funcName.substr(dotPos + 1, secondDot - dotPos - 1);
                funcName = funcName.substr(secondDot + 1);
            } else {
                funcName = funcName.substr(dotPos + 1);
            }
        } else {
            objectName = firstPart;
            funcName = funcName.substr(dotPos + 1);
        }
        hasObject = !objectName.empty();
    }

    // Function declaration
    if (isPublic) {
        output << "public ";
    }
    if (hasObject) {
        output << "function " << objectName << "." << funcName << "() {\n";
    } else {
        output << "function " << funcName << "() {\n";
    }

    // Generate function body - simplified version
    output << "  // Decompilation not fully implemented yet\n";
    output << "  // Function: " << func.name << "\n";
    output << "  // Opcodes: " << func.bytecode.size() << " bytes\n";

    auto it = functionInstructions.find(func.name);
    if (it != functionInstructions.end()) {
        const auto& instructions = it->second;
        for (const auto& inst : instructions) {
            output << "  // " << opcode::OpcodeToString(inst.op);
            if (!inst.operandString.empty()) {
                output << " \"" << inst.operandString << "\"";
            } else if (inst.operandInt != 0) {
                output << " " << inst.operandInt;
            }
            output << "\n";
        }
    }

    output << "}";

    return output.str();
}

std::string GS2Decompiler::generateStatement(uint32_t& opIndex, int indent, const FunctionInfo& func) {
    return "";
}

std::string GS2Decompiler::generateExpression(uint32_t& opIndex, const FunctionInfo& func) {
    return "";
}

std::string GS2Decompiler::getStringFromTable(int32_t index) {
    if (index >= 0 && index < static_cast<int32_t>(stringTable.size())) {
        return stringTable[index];
    }
    return "";
}

std::string GS2Decompiler::indentString(int level) {
    return std::string(level * 2, ' ');
}

bool GS2Decompiler::isJumpTarget(uint32_t opIndex) const {
    return jumpTargets.find(opIndex) != jumpTargets.end();
}

} // namespace gs2decompiler
