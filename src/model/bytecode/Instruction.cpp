#include "Instruction.hpp"

#include <cstdio>

#include "OpcodeTable.hpp"

namespace JByteDom::model::bytecode {
    namespace {
        auto resolveConstantPoolEntry(
            const clazz::ClassMetadata& classMetadata,
            const u16 constantPoolIndex,
            const int recursionDepth,
            char* outputBuffer,
            const usize outputBufferSize
        ) -> void {
            if (outputBufferSize == 0) {
                return;
            }

            outputBuffer[0] = '\0';

            if (constantPoolIndex == 0 || constantPoolIndex >= classMetadata.constantPool.getSize()) {
                snprintf(outputBuffer, outputBufferSize, "<invalid>");
                return;
            }

            if (recursionDepth > 8) {
                snprintf(outputBuffer, outputBufferSize, "<max depth>");
                return;
            }

            const auto& constantPoolEntry = classMetadata.constantPool[constantPoolIndex];
            switch (constantPoolEntry.tag) {
                case clazz::ConstantPoolEntryTag::Utf8: {
                    if (constantPoolEntry.utf8Str) {
                        snprintf(outputBuffer, outputBufferSize, "\"%s\"", constantPoolEntry.utf8Str);
                    } else {
                        snprintf(outputBuffer, outputBufferSize, "\"\"");
                    }
                    break;
                }
                case clazz::ConstantPoolEntryTag::Integer: {
                    snprintf(outputBuffer, outputBufferSize, "%d", (int) constantPoolEntry.intValue);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Float: {
                    float floatValue;
                    memcpy(&floatValue, &constantPoolEntry.intValue, 4);
                    snprintf(outputBuffer, outputBufferSize, "%f", floatValue);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Long: {
                    snprintf(outputBuffer, outputBufferSize, "%lldL", (long long) constantPoolEntry.longValue);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Double: {
                    double doubleValue;
                    memcpy(&doubleValue, &constantPoolEntry.longValue, 8);
                    snprintf(outputBuffer, outputBufferSize, "%f", doubleValue);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Class: {
                    const u16 nameIndex = constantPoolEntry.nameIndex;
                    if (nameIndex > 0 && nameIndex < classMetadata.constantPool.getSize()) {
                        const auto& nameEntry = classMetadata.constantPool[nameIndex];
                        if (nameEntry.tag == clazz::ConstantPoolEntryTag::Utf8 && nameEntry.utf8Str) {
                            const char* rawName = nameEntry.utf8Str;
                            char* writePointer = outputBuffer;
                            for (const char* readPointer = rawName; *readPointer && (usize) (writePointer - outputBuffer) < outputBufferSize - 1; ++readPointer) {
                                *writePointer++ = (*readPointer == '/') ? '.' : *readPointer;
                            }
                            *writePointer = '\0';
                            break;
                        }
                    }
                    snprintf(outputBuffer, outputBufferSize, "Class #%d", nameIndex);
                    break;
                }
                case clazz::ConstantPoolEntryTag::String: {
                    const u16 utf8Index = constantPoolEntry.nameIndex;
                    if (utf8Index > 0 && utf8Index < classMetadata.constantPool.getSize()) {
                        char tempBuffer[128];
                        resolveConstantPoolEntry(classMetadata, utf8Index, recursionDepth + 1, tempBuffer, sizeof(tempBuffer));
                        snprintf(outputBuffer, outputBufferSize, "%s", tempBuffer);
                    } else {
                        snprintf(outputBuffer, outputBufferSize, "String #%d", utf8Index);
                    }
                    break;
                }
                case clazz::ConstantPoolEntryTag::Fieldref: {
                    char classNameBuffer[256];
                    char nameAndTypeBuffer[256];
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.classIndex, recursionDepth + 1, classNameBuffer, sizeof(classNameBuffer));
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameAndTypeIndex, recursionDepth + 1, nameAndTypeBuffer, sizeof(nameAndTypeBuffer));
                    snprintf(outputBuffer, outputBufferSize, "%s.%s", classNameBuffer, nameAndTypeBuffer);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Methodref: {
                    char classNameBuffer[256];
                    char nameAndTypeBuffer[256];
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.classIndex, recursionDepth + 1, classNameBuffer, sizeof(classNameBuffer));
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameAndTypeIndex, recursionDepth + 1, nameAndTypeBuffer, sizeof(nameAndTypeBuffer));
                    snprintf(outputBuffer, outputBufferSize, "%s.%s", classNameBuffer, nameAndTypeBuffer);
                    break;
                }
                case clazz::ConstantPoolEntryTag::InterfaceMethodref: {
                    char classNameBuffer[256];
                    char nameAndTypeBuffer[256];
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.classIndex, recursionDepth + 1, classNameBuffer, sizeof(classNameBuffer));
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameAndTypeIndex, recursionDepth + 1, nameAndTypeBuffer, sizeof(nameAndTypeBuffer));
                    snprintf(outputBuffer, outputBufferSize, "%s.%s", classNameBuffer, nameAndTypeBuffer);
                    break;
                }
                case clazz::ConstantPoolEntryTag::NameAndType: {
                    char nameBuffer[128];
                    char descriptorBuffer[128];
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameIndex, recursionDepth + 1, nameBuffer, sizeof(nameBuffer));
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.descriptorIndex, recursionDepth + 1, descriptorBuffer, sizeof(descriptorBuffer));

                    const char* nameStart = nameBuffer;
                    if (nameStart[0] == '"') {
                        nameStart++;
                    }

                    usize nameLen = strlen(nameStart);
                    if (nameLen > 0 && nameStart[nameLen - 1] == '"') {
                        nameLen--;
                    }

                    const char* descriptorStart = descriptorBuffer;
                    if (descriptorStart[0] == '"') {
                        descriptorStart++;
                    }

                    usize descriptorLen = strlen(descriptorStart);
                    if (descriptorLen > 0 && descriptorStart[descriptorLen - 1] == '"') {
                        descriptorLen--;
                    }

                    snprintf(outputBuffer, outputBufferSize, "%.*s%.*s", (int) nameLen, nameStart, (int) descriptorLen, descriptorStart);
                    break;
                }
                case clazz::ConstantPoolEntryTag::MethodHandle: {
                    snprintf(outputBuffer, outputBufferSize, "MethodHandle #%d.%d", constantPoolEntry.referenceType, constantPoolEntry.referenceIndex);
                    break;
                }
                case clazz::ConstantPoolEntryTag::MethodType: {
                    char descriptorBuffer[128];
                    resolveConstantPoolEntry(classMetadata, constantPoolEntry.descriptorIndex, recursionDepth + 1, descriptorBuffer, sizeof(descriptorBuffer));
                    snprintf(outputBuffer, outputBufferSize, "MethodType %s", descriptorBuffer);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Dynamic: {
                    snprintf(outputBuffer, outputBufferSize, "Dynamic #%d.#%d", constantPoolEntry.bootstrapAttributeIndex, constantPoolEntry.nameAndTypeIndex);
                    break;
                }
                case clazz::ConstantPoolEntryTag::InvokeDynamic: {
                    snprintf(outputBuffer, outputBufferSize, "Dynamic #%d.#%d", constantPoolEntry.bootstrapAttributeIndex, constantPoolEntry.nameAndTypeIndex);
                    break;
                }
                case clazz::ConstantPoolEntryTag::Module: {
                    if (constantPoolEntry.nameIndex > 0 && constantPoolEntry.nameIndex < classMetadata.constantPool.getSize()) {
                        char nameBuffer[128];
                        resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameIndex, recursionDepth + 1, nameBuffer, sizeof(nameBuffer));
                        snprintf(outputBuffer, outputBufferSize, "Module %s", nameBuffer);
                    } else {
                        snprintf(outputBuffer, outputBufferSize, "Module #%d", constantPoolEntry.nameIndex);
                    }
                    break;
                }
                case clazz::ConstantPoolEntryTag::Package: {
                    if (constantPoolEntry.nameIndex > 0 && constantPoolEntry.nameIndex < classMetadata.constantPool.getSize()) {
                        char nameBuffer[128];
                        resolveConstantPoolEntry(classMetadata, constantPoolEntry.nameIndex, recursionDepth + 1, nameBuffer, sizeof(nameBuffer));
                        snprintf(outputBuffer, outputBufferSize, "Package %s", nameBuffer);
                    } else {
                        snprintf(outputBuffer, outputBufferSize, "Package #%d", constantPoolEntry.nameIndex);
                    }
                    break;
                }
                default: {
                    snprintf(outputBuffer, outputBufferSize, "tag=%d", constantPoolEntry.tag);
                    break;
                }
            }
        }
    }

    Instruction::~Instruction() {
        this->modifiedOperands.clear();
        this->switchOperandBytes.clear();
    }

    Instruction::Instruction(const Instruction& other):
        opcode(other.opcode),
        operand(other.operand),
        offset(other.offset),
        length(other.length),
        isWide(other.isWide),
        modifyIndicator(other.modifyIndicator)
    {
        if (other.switchOperandBytes.getSize() > 0) {
            this->switchOperandBytes.reserve(other.switchOperandBytes.getSize());
            for (usize i = 0; i < other.switchOperandBytes.getSize(); ++i) {
                this->switchOperandBytes.push(other.switchOperandBytes[i]);
            }
        }

        if (other.modifyIndicator) {
            this->modifiedOperands.reserve(other.modifiedOperands.getSize());
            for (usize i = 0; i < other.modifiedOperands.getSize(); ++i) {
                this->modifiedOperands.push(other.modifiedOperands[i]);
            }
        }
    }
    Instruction& Instruction::operator = (const Instruction &other) {
        if (this != &other) {
            this->switchOperandBytes.clear();
            this->modifiedOperands.clear();

            this->opcode = other.opcode;
            this->operand = other.operand;
            this->offset = other.offset;
            this->length = other.length;
            this->isWide = other.isWide;
            this->modifyIndicator = other.modifyIndicator;

            if (other.switchOperandBytes.getSize() > 0) {
                this->switchOperandBytes.reserve(other.switchOperandBytes.getSize());
                for (usize i = 0; i < other.switchOperandBytes.getSize(); ++i) {
                    this->switchOperandBytes.push(other.switchOperandBytes[i]);
                }
            }

            if (other.modifyIndicator) {
                this->modifiedOperands.reserve(other.modifiedOperands.getSize());
                for (usize i = 0; i < other.modifiedOperands.getSize(); ++i) {
                    this->modifiedOperands.push(other.modifiedOperands[i]);
                }
            }
        }

        return *this;
    }

    Instruction::Instruction(Instruction &&other) noexcept:
        opcode(other.opcode),
        operand(std::move(other.operand)),
        offset(other.offset),
        length(other.length),
        isWide(other.isWide),
        switchOperandBytes(std::move(other.switchOperandBytes)),
        modifyIndicator(other.modifyIndicator),
        modifiedOperands(std::move(other.modifiedOperands))
    {
        other.opcode = nullptr;
        other.offset = 0;
        other.length = 0;
        other.isWide = false;
        other.modifyIndicator = false;
        std::memset(&other.operand, 0, sizeof(other.operand));
    }
    Instruction& Instruction::operator = (Instruction &&other) noexcept {
        if (this != &other) {
            this->switchOperandBytes.clear();
            this->modifiedOperands.clear();

            this->opcode = other.opcode;
            this->operand = std::move(other.operand);
            this->offset = other.offset;
            this->length = other.length;
            this->isWide = other.isWide;
            this->switchOperandBytes = std::move(other.switchOperandBytes);
            this->modifyIndicator = other.modifyIndicator;
            this->modifiedOperands = std::move(other.modifiedOperands);

            other.opcode = nullptr;
            other.offset = 0;
            other.length = 0;
            other.modifyIndicator = false;
            std::memset(&other.operand, 0, sizeof(other.operand));
        }

        return *this;
    }

    auto Instruction::modifyOperand(
        const u8 *newData,
        const u8 newLen
    ) -> void {
        this->modifiedOperands.clear();
        for (u8 i = 0; i < newLen; ++i) {
            this->modifiedOperands.push(newData[i]);
        }
        this->modifyIndicator = true;
    }

    auto Instruction::getEffectiveOperandBytes() const -> const u8 * {
        return this->modifyIndicator ? this->modifiedOperands.peekFirst() : this->operand.bytes;
    }
    auto Instruction::getEffectiveOperandLength() const -> u8 {
        return this->modifyIndicator ? (u8) this->modifiedOperands.getSize() : this->operand.length;
    }

    auto Instruction::getDisplayFullName(
        const clazz::ClassMetadata& classMetadata
    ) const -> const char* {
        thread_local char displayBuffer[512];
        displayBuffer[0] = '\0';

        const char* mnemonic = opcode ? opcode->mnemonic : "<unknown>";
        strcpy(displayBuffer, mnemonic);

        if (operand.type != OperandType::None && operand.length > 0) {
            bool isConstantPoolIndex = false;
            u16 resolvedConstantPoolIndex = 0;

            if (opcode != nullptr &&
                (opcode->operandTemplate == OperandTemplate::ConstantPoolIndex ||
                 opcode->operandTemplate == OperandTemplate::WideConstantPoolIndex)) {
                isConstantPoolIndex = true;
                resolvedConstantPoolIndex = (u16) operand.value;
            }

            if (isConstantPoolIndex && resolvedConstantPoolIndex > 0 && resolvedConstantPoolIndex < classMetadata.constantPool.getSize()) {
                char entryDisplayBuffer[256];
                resolveConstantPoolEntry(classMetadata, resolvedConstantPoolIndex, 0, entryDisplayBuffer, sizeof(entryDisplayBuffer));
                const usize currentLength = strlen(displayBuffer);
                snprintf(displayBuffer + currentLength, sizeof(displayBuffer) - currentLength, " %s", entryDisplayBuffer);
            } else {
                switch (operand.type) {
                    case OperandType::Byte: {
                        char operandBuffer[64];
                        snprintf(operandBuffer, sizeof(operandBuffer), " %d", (int) operand.value);
                        strcat(displayBuffer, operandBuffer);
                        break;
                    }
                    case OperandType::Short: {
                        char operandBuffer[64];
                        snprintf(operandBuffer, sizeof(operandBuffer), " %d", (int) operand.value);
                        strcat(displayBuffer, operandBuffer);
                        break;
                    }
                    case OperandType::Int: {
                        char operandBuffer[64];
                        snprintf(operandBuffer, sizeof(operandBuffer), " %d", (int) operand.value);
                        strcat(displayBuffer, operandBuffer);
                        break;
                    }
                    case OperandType::Long: {
                        char operandBuffer[64];
                        snprintf(operandBuffer, sizeof(operandBuffer), " %lld", (long long) operand.value);
                        strcat(displayBuffer, operandBuffer);
                        break;
                    }
                    case OperandType::Switch: {
                        strcat(displayBuffer, " (switch)");
                        break;
                    }
                    default: break;
                }
            }
        }

        if (modifyIndicator) {
            strcat(displayBuffer, " *modified*");
        }

        return displayBuffer;
    }

    auto Instruction::make(Opcodes opcodeType) -> Instruction {
        const auto* opcode = &OPCODE_TABLE[(u8) opcodeType];
        auto instruction = Instruction();
        instruction.opcode = opcode;
        instruction.operand.type = OperandType::None;
        instruction.operand.length = 0;
        instruction.operand.value = 0;
        instruction.isWide = false;
        instruction.length = 1;
        return instruction;
    }
    auto Instruction::make(Opcodes opcodeType, const u8 operand) -> Instruction {
        const auto* opcode = &OPCODE_TABLE[(u8) opcodeType];
        auto instruction = Instruction();
        instruction.opcode = opcode;
        instruction.operand.type = OperandType::Byte;
        instruction.operand.length = 1;
        instruction.operand.bytes[0] = operand;
        instruction.operand.value = operand;
        instruction.isWide = false;
        instruction.length = 2;
        return instruction;
    }
    auto Instruction::make(Opcodes opcodeType, const u16 operand) -> Instruction {
        const auto* opcode = &OPCODE_TABLE[(u8) opcodeType];
        auto instruction = Instruction();
        instruction.opcode = opcode;
        instruction.operand.type = OperandType::Short;
        instruction.operand.length = 2;
        instruction.operand.bytes[0] = (u8) (operand >> 8 & 0xFF);
        instruction.operand.bytes[1] = (u8) (operand & 0xFF);
        instruction.operand.value = operand;
        instruction.isWide = false;
        instruction.length = 3;
        return instruction;
    }
    auto Instruction::make(Opcodes opcodeType, const u32 operand) -> Instruction {
        const auto* opcode = &OPCODE_TABLE[(u8) opcodeType];
        auto instruction = Instruction();
        instruction.opcode = opcode;
        instruction.operand.type = OperandType::Int;
        instruction.operand.length = 4;
        instruction.operand.bytes[0] = (u8) (operand >> 24 & 0xFF);
        instruction.operand.bytes[1] = (u8) (operand >> 16 & 0xFF);
        instruction.operand.bytes[2] = (u8) (operand >> 8 & 0xFF);
        instruction.operand.bytes[3] = (u8) (operand & 0xFF);
        instruction.operand.value = operand;
        instruction.isWide = false;
        instruction.length = 5;
        return instruction;
    }
    auto Instruction::make(
        Opcodes opcodeType,
        const u8* operands,
        const u8 operandsLen
    ) -> Instruction {
        const auto* opcode = &OPCODE_TABLE[(u8) opcodeType];
        auto instruction = Instruction();

        instruction.opcode = opcode;
        instruction.operand.type = OperandType::Switch;
        instruction.operand.length = operandsLen;
        for (u8 i = 0; i < operandsLen && i < 8; ++i) {
            instruction.operand.bytes[i] = operands[i];
        }

        if (operandsLen == 1) {
            instruction.operand.value = operands[0];
        } else if (operandsLen == 2) {
            instruction.operand.value = (u16) operands[0] << 8 | operands[1];
        } else if (operandsLen == 4) {
            instruction.operand.value = (u32) operands[0] << 24 | (u32) operands[1] << 16 | (u32) operands[2] << 8 | operands[3];
        }

        instruction.isWide = false;
        instruction.length = 1 + operandsLen;

        return instruction;
    }
}
