#include "InstructionsModifier.hpp"

#include "../../preprocess/parse/BytecodesParser.hpp"
#include "../../util/ByteUtils.hpp"
#include "../compute/StackFrameComputer.hpp"

namespace JByteDom::runtime::modify {
    namespace {
        auto correctExceptionTable(
            model::clazz::MethodInfo& methodInfo,
            const u32* oldToNew,
            const u32 oldCodeLen
        ) -> bool {
            u8* data = methodInfo.exceptionTableBytes.getFirst();
            const usize byteLen = methodInfo.exceptionTableBytes.getSize();

            if (!data || byteLen == 0) {
                return true;
            }

            if (byteLen % 8 != 0) {
                return false;
            }

            const usize entryCount = byteLen / 8;
            for (usize i = 0; i < entryCount; ++i) {
                u8* entry = data + i * 8;

                const u16 oldStart = (u16) (entry[0] << 8 | entry[1]);
                if (oldStart < oldCodeLen) {
                    const u32 newStart = oldToNew[oldStart];
                    if (newStart == 0xFFFFFFFF) {
                        return false;
                    }
                    entry[0] = newStart >> 8 & 0xFF;
                    entry[1] = newStart & 0xFF;
                }

                const u16 oldEnd = (u16) (entry[2] << 8 | entry[3]);
                if (oldEnd < oldCodeLen) {
                    const u32 newEnd = oldToNew[oldEnd];
                    if (newEnd == 0xFFFFFFFF) {
                        return false;
                    }
                    entry[2] = newEnd >> 8 & 0xFF;
                    entry[3] = newEnd & 0xFF;
                }

                const u16 oldHandler = (u16) (entry[4] << 8 | entry[5]);
                if (oldHandler < oldCodeLen) {
                    const u32 newHandler = oldToNew[oldHandler];
                    if (newHandler == 0xFFFFFFFF) {
                        return false;
                    }
                    entry[4] = newHandler >> 8 & 0xFF;
                    entry[5] = newHandler & 0xFF;
                }
            }

            return true;
        }

        auto relocateMethodCode(
            model::clazz::MethodInfo& methodInfo,
            model::common::ElasticArray<model::bytecode::Instruction>& instructions
        ) -> bool {
            const u32 oldCodeLen = methodInfo.codeLength;
            const usize instructionCount = instructions.getSize();
            if (oldCodeLen == 0 || instructionCount == 0) {
                return true;
            }

            const auto oldToNew = (u32*) malloc(oldCodeLen * sizeof(u32));
            const auto newStarts = (u32*) malloc(instructionCount * sizeof(u32));
            if (!oldToNew || !newStarts) {
                free(oldToNew);
                free(newStarts);
                return false;
            }

            memset(oldToNew, 0xFF, oldCodeLen * sizeof(u32));

            u32 newOffset = 0;
            for (usize i = 0; i < instructionCount; ++i) {
                auto& instruction = instructions[i];
                newStarts[i] = newOffset;
                if (instruction.offset < oldCodeLen) {
                    oldToNew[instruction.offset] = newOffset;
                }

                u32 instNewLen;
                if (instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::Switch) {
                    const u32 oldPadding = (4 - (instruction.offset + 1) % 4) % 4;
                    const u8* base = instruction.switchOperandBytes.peekFirst();
                    if (!base) {
                        free(oldToNew);
                        free(newStarts);
                        return false;
                    }

                    u32 pos = oldPadding;
                    pos += 4;
                    u32 tableLen = 0;
                    if (instruction.opcode->value == (u8)model::bytecode::Opcodes::tableswitch) {
                        const u8* rp = base + pos;
                        const u32 low = util::readU4(rp);
                        const u32 high = util::readU4(rp);
                        tableLen = (high - low + 1) * 4;
                    } else {
                        const u8* rp = base + pos;
                        const u32 npairs = util::readU4(rp);
                        tableLen = npairs * 8;
                    }

                    const u32 newPadding = (4 - (newOffset + 1) % 4) % 4;
                    const u32 operandLen = newPadding + 4 + 8 + tableLen;
                    instNewLen = 1 + operandLen;
                } else {
                    instNewLen = (instruction.isWide ? 1 : 0) + 1 + instruction.getEffectiveOperandLength();
                }
                newOffset += instNewLen;
            }

            for (usize i = 0; i < instructionCount; ++i) {
                auto& instruction = instructions[i];

                if (instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::BranchOffset || instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::WideBranchOffset) {
                    const i32 oldBranchOff = (i32) instruction.operand.value;
                    const u32 oldTarget = instruction.offset + oldBranchOff;
                    if (oldTarget >= oldCodeLen) {
                        continue;
                    }

                    const u32 newTarget = oldToNew[oldTarget];
                    if (newTarget == 0xFFFFFFFF) {
                        continue;
                    }

                    const u32 newStart = newStarts[i];
                    const i32 newDelta = (i32) (newTarget - newStart);

                    if (instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::BranchOffset) {
                        const u16 delta16 = (u16) newDelta;
                        instruction.operand.bytes[0] = delta16 >> 8 & 0xFF;
                        instruction.operand.bytes[1] = delta16 & 0xFF;
                    } else {
                        const u32 delta32 = (u32) newDelta;
                        instruction.operand.bytes[0] = delta32 >> 24 & 0xFF;
                        instruction.operand.bytes[1] = delta32 >> 16 & 0xFF;
                        instruction.operand.bytes[2] = delta32 >> 8 & 0xFF;
                        instruction.operand.bytes[3] = delta32 & 0xFF;
                    }
                } else if (instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::Switch) {
                    const u8* oldBase = instruction.switchOperandBytes.peekFirst();
                    if (!oldBase) {
                        continue;
                    }

                    const u32 oldStart = instruction.offset;
                    const u32 newStart = newStarts[i];
                    const u32 oldPadding = (4 - (oldStart + 1) % 4) % 4;
                    u32 pos = oldPadding;

                    const u8* rp = oldBase + pos;
                    const i32 oldDefault = (i32) util::readU4(rp);
                    pos += 4;

                    i32 newDefault = 0;
                    const u32 oldTargetDefault = oldStart + oldDefault;
                    if (oldTargetDefault < oldCodeLen && oldToNew[oldTargetDefault] != 0xFFFFFFFF) {
                        const u32 newTarget = oldToNew[oldTargetDefault];
                        newDefault = (i32) (newTarget - newStart);
                    }

                    if (instruction.opcode->value == (u8) model::bytecode::Opcodes::tableswitch) {
                        const u32 low = util::readU4(rp);
                        const u32 high = util::readU4(rp);
                        pos += 8;
                        const u32 count = high - low + 1;

                        const auto oldOffsets = (u32*) malloc(count * sizeof(u32));
                        if (!oldOffsets) {
                            free(oldToNew);
                            free(newStarts);
                            return false;
                        }

                        for (u32 j = 0; j < count; ++j) {
                            oldOffsets[j] = util::readU4(rp);
                            pos += 4;
                        }

                        instruction.switchOperandBytes.clear();
                        const u32 newPadding = (4 - (newStart + 1) % 4) % 4;
                        for (u32 p = 0; p < newPadding; ++p) {
                            instruction.switchOperandBytes.push(0);
                        }

                        util::writeU4(instruction.switchOperandBytes, (u32) newDefault);
                        util::writeU4(instruction.switchOperandBytes, low);
                        util::writeU4(instruction.switchOperandBytes, high);

                        for (u32 j = 0; j < count; ++j) {
                            i32 newOff = 0;
                            const u32 oldTarget = oldStart + oldOffsets[j];
                            if (oldTarget < oldCodeLen && oldToNew[oldTarget] != 0xFFFFFFFF) {
                                const u32 newTarget = oldToNew[oldTarget];
                                newOff = (i32) (newTarget - newStart);
                            }
                            util::writeU4(instruction.switchOperandBytes, (u32) newOff);
                        }
                        free(oldOffsets);
                    } else {
                        const u32 npairs = util::readU4(rp);
                        pos += 4;

                        const auto keys = (u32*) malloc(npairs * sizeof(u32));
                        const auto values = (u32*) malloc(npairs * sizeof(u32));
                        if (!keys || !values) {
                            free(keys);
                            free(values);
                            free(oldToNew);
                            free(newStarts);
                            return false;
                        }

                        for (u32 j = 0; j < npairs; ++j) {
                            keys[j] = util::readU4(rp);
                            values[j] = util::readU4(rp);
                            pos += 8;
                        }

                        instruction.switchOperandBytes.clear();
                        const u32 newPadding = (4 - (newStart + 1) % 4) % 4;
                        for (u32 p = 0; p < newPadding; ++p) {
                            instruction.switchOperandBytes.push(0);
                        }

                        util::writeU4(instruction.switchOperandBytes, (u32) newDefault);
                        util::writeU4(instruction.switchOperandBytes, npairs);

                        for (u32 j = 0; j < npairs; ++j) {
                            util::writeU4(instruction.switchOperandBytes, keys[j]);
                            i32 newValue = 0;
                            const u32 oldTarget = oldStart + values[j];
                            if (oldTarget < oldCodeLen && oldToNew[oldTarget] != 0xFFFFFFFF) {
                                const u32 newTarget = oldToNew[oldTarget];
                                newValue = (i32) (newTarget - newStart);
                            }
                            util::writeU4(instruction.switchOperandBytes, (u32) newValue);
                        }

                        free(keys);
                        free(values);
                    }
                }
            }

            for (usize i = 0; i < instructionCount; ++i) {
                instructions[i].offset = newStarts[i];
            }

            if (!correctExceptionTable(methodInfo, oldToNew, oldCodeLen)) {
                free(oldToNew);
                free(newStarts);
                return false;
            }

            free(oldToNew);
            free(newStarts);
            return true;
        }
    }

    InstructionsModifier::InstructionsModifier(
        model::clazz::ClassMetadata& classMetadata,
        model::clazz::MethodInfo& methodInfo
    ):
        classMetadata(classMetadata),
        methodInfo(methodInfo),
        maxStack(methodInfo.maxStack),
        maxLocals(methodInfo.maxLocals)
    {
        if (methodInfo.hasCode && methodInfo.bytecodes.getSize() > 0) {
            instructions = preprocess::parse::BytecodesParser::parse(
                methodInfo.bytecodes.peekFirst(),
                methodInfo.bytecodes.getSize()
            );
        }

        this->modified = false;
    }

    auto InstructionsModifier::insertHead(const model::bytecode::Instruction& instruction) -> bool {
        instructions.insertAt(0, instruction);
        modified = true;
        return true;
    }
    auto InstructionsModifier::insertHead(const std::initializer_list<model::bytecode::Instruction> instructionList) -> bool {
        for (auto it = instructionList.end(); it != instructionList.begin(); ) {
            --it;
            if (!insertHead(*it)) {
                return false;
            }
        }
        return true;
    }
    auto InstructionsModifier::insertTail(const model::bytecode::Instruction& instruction) -> bool {
        instructions.push(instruction);
        modified = true;
        return true;
    }
    auto InstructionsModifier::insertTail(const std::initializer_list<model::bytecode::Instruction> instructionList) -> bool {
        for (const auto& inst : instructionList) {
            if (!insertTail(inst)) {
                return false;
            }
        }
        return true;
    }

    auto InstructionsModifier::insertBefore(const usize index, const model::bytecode::Instruction& instruction) -> bool {
        if (index > instructions.getSize()) {
            return false;
        }

        instructions.insertAt(index, instruction);
        modified = true;
        return true;
    }
    auto InstructionsModifier::insertBefore(const usize index, const std::initializer_list<model::bytecode::Instruction> instructionList) -> bool {
        for (auto it = instructionList.end(); it != instructionList.begin(); ) {
            --it;
            if (!insertBefore(index, *it)) {
                return false;
            }
        }
        return true;
    }
    auto InstructionsModifier::insertAfter(const usize index, const model::bytecode::Instruction& instruction) -> bool {
        if (index >= instructions.getSize()) {
            return false;
        }

        instructions.insertAt(index + 1, instruction);
        modified = true;
        return true;
    }
    auto InstructionsModifier::insertAfter(const usize index, const std::initializer_list<model::bytecode::Instruction> instructionList) -> bool {
        for (const auto& inst : instructionList) {
            if (!insertAfter(index, inst)) {
                return false;
            }
        }
        return true;
    }

    auto InstructionsModifier::removeHead() -> bool {
        if (instructions.getSize() == 0) {
            return false;
        }

        instructions.removeAt(0);
        modified = true;
        return true;
    }
    auto InstructionsModifier::removeTail() -> bool {
        if (instructions.getSize() == 0) {
            return false;
        }

        instructions.removeAt(instructions.getSize() - 1);
        modified = true;
        return true;
    }
    auto InstructionsModifier::removeAt(const usize index) -> bool {
        if (index >= instructions.getSize()) {
            return false;
        }

        instructions.removeAt(index);
        modified = true;
        return true;
    }

    auto InstructionsModifier::makeLdcString(const char* str) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addStringEntry(str);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        if (index >= 256) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::ldc_w, (u32)index);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::ldc, (u8)index);
    }
    auto InstructionsModifier::makeGetStatic(const char* className, const char* fieldName, const char* descriptor) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addFieldrefEntry(className, fieldName, descriptor);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::getstatic, index);
    }
    auto InstructionsModifier::makePutStatic(const char* className, const char* fieldName, const char* descriptor) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addFieldrefEntry(className, fieldName, descriptor);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::putstatic, index);
    }
    auto InstructionsModifier::makeInvokeVirtual(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addMethodrefEntry(className, methodName, descriptor);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::invokevirtual, index);
    }
    auto InstructionsModifier::makeInvokeSpecial(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addMethodrefEntry(className, methodName, descriptor);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::invokespecial, index);
    }
    auto InstructionsModifier::makeInvokeStatic(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction {
        const u16 index = classMetadata.addMethodrefEntry(className, methodName, descriptor);
        if (index == 0) {
            return model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
        }
        return model::bytecode::Instruction::make(model::bytecode::Opcodes::invokestatic, index);
    }

    auto InstructionsModifier::insertPrintlnHead(const char* str) -> bool {
        return insertHead({
            makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
            makeLdcString(str),
            makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
        });
    }
    auto InstructionsModifier::insertPrintlnTail(const char* str) -> bool {
        return insertTail({
            makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
            makeLdcString(str),
            makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
        });
    }
    auto InstructionsModifier::insertPrintlnBefore(const usize index, const char* str) -> bool {
        return insertBefore(index, {
            makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
            makeLdcString(str),
            makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
        });
    }
    auto InstructionsModifier::insertPrintlnAfter(const usize index, const char* str) -> bool {
        return insertAfter(index, {
            makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
            makeLdcString(str),
            makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
        });
    }

    auto InstructionsModifier::applyAllModification() -> bool {
        if (!this->modified) {
            return true;
        }

        if (this->instructions.getSize() == 0) {
            this->methodInfo.hasCode = false;
            this->methodInfo.bytecodes.clear();
            this->methodInfo.codeLength = 0;
            this->methodInfo.maxStack = 0;
            this->methodInfo.maxLocals = 0;
            this->methodInfo.exceptionTableBytes.clear();
            this->methodInfo.codeAttributes.clear();

            this->modified = false;
            return true;
        }

        this->methodInfo.hasCode = true;
        if (!relocateMethodCode(this->methodInfo, this->instructions)) {
            return false;
        }

        this->methodInfo.bytecodes = preprocess::parse::BytecodesParser::emit(this->instructions);
        this->methodInfo.codeLength = (u32) this->methodInfo.bytecodes.getSize();

        const auto frameInfo = compute::StackFrameComputer::computeFrameInfo(this->instructions);
        this->methodInfo.maxStack = frameInfo.maxStack;
        this->methodInfo.maxLocals = frameInfo.maxLocals;

        this->methodInfo.exceptionTableBytes.clear();
        this->methodInfo.codeAttributes.clear();

        this->modified = false;
        return true;
    }
}
