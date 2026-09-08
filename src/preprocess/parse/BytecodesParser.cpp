#include "BytecodesParser.hpp"

#include "../../model/bytecode/OpcodeTable.hpp"
#include "../../util/ByteUtils.hpp"

namespace JByteDom::preprocess::parse {
    namespace {
        auto parseOperandByTemplate(
            const model::bytecode::OperandTemplate operandTemplate,
            const u8*& bytecode,
            model::bytecode::Operand& operand
        ) -> void {
            switch (operandTemplate) {
                using namespace model::bytecode;
                case OperandTemplate::None: {
                    operand.type = OperandType::None;
                    operand.length = 0;
                    operand.value = 0;
                    break;
                }
                case OperandTemplate::ByteImm: {
                    operand.type = OperandType::Byte;
                    operand.length = 1;
                    operand.bytes[0] = util::readU1(bytecode);
                    operand.value = operand.bytes[0];
                    break;
                }
                case OperandTemplate::ShortImm: {
                    operand.type = OperandType::Short;
                    operand.length = 2;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.value = util::readU2(bytecode);
                    break;
                }
                case OperandTemplate::LocalVarIndex: {
                    operand.type = OperandType::Byte;
                    operand.length = 1;
                    operand.bytes[0] = util::readU1(bytecode);
                    operand.value = operand.bytes[0];
                    break;
                }
                case OperandTemplate::WideLocalVarIndex: {
                    operand.type = OperandType::WideShort;
                    operand.length = 2;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.value = util::readU2(bytecode);
                    break;
                }
                case OperandTemplate::ConstantPoolIndex: {
                    operand.type = OperandType::Short;
                    operand.length = 2;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.value = util::readU2(bytecode);
                    break;
                }
                case OperandTemplate::WideConstantPoolIndex: {
                    operand.type = OperandType::Int;
                    operand.length = 4;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.bytes[2] = bytecode[2];
                    operand.bytes[3] = bytecode[3];
                    operand.value = util::readU4(bytecode);
                    break;
                }
                case OperandTemplate::BranchOffset: {
                    operand.type = OperandType::Short;
                    operand.length = 2;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.value = (i16) util::readU2(bytecode);
                    break;
                }
                case OperandTemplate::WideBranchOffset: {
                    operand.type = OperandType::Int;
                    operand.length = 4;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.bytes[2] = bytecode[2];
                    operand.bytes[3] = bytecode[3];
                    operand.value = (i32) util::readU4(bytecode);
                    break;
                }
                case OperandTemplate::Switch: {
                    operand.type = OperandType::Switch;
                    operand.length = 0;
                    operand.value = 0;
                    break;
                }
                case OperandTemplate::MultiANewArray: {
                    operand.type = OperandType::MultiANewArray;
                    operand.length = 3;
                    operand.bytes[0] = bytecode[0];
                    operand.bytes[1] = bytecode[1];
                    operand.bytes[2] = bytecode[2];
                    operand.value = (u64) util::readU2(bytecode) << 8 | util::readU1(bytecode);
                    break;
                }
            }
        }
    }

    namespace {
        auto computeSwitchLength(
            const u8* bytecodeStart,
            const u8 opcode
        ) -> u32 {
            const u8* bytecode = bytecodeStart;
            const u32 offset = (u32) (bytecode - bytecodeStart);
            const u32 padding = (4 - offset % 4) % 4;
            bytecode += padding;
            bytecode += 4;

            if (opcode == (u8) model::bytecode::Opcodes::tableswitch) {
                const u32 low = util::readU4(bytecode);
                const u32 high = util::readU4(bytecode);
                const u32 pairCount = high - low + 1;
                bytecode += 4 * pairCount;
            } else {
                const u32 npairs = util::readU4(bytecode);
                bytecode += 8 * npairs;
            }

            return (u32) (bytecode - bytecodeStart);
        }

        auto readSwitchData(
            model::common::ElasticArray<u8>& bytes,
            const u8* bytecodeStart,
            const u32 bytecodeLen
        ) -> void {
            bytes.reserve(bytecodeLen);
            for (u32 i = 0; i < bytecodeLen; ++i) {
                bytes.push(bytecodeStart[i]);
            }
        }
    }

    namespace {
        auto parseSwitchInstruction(
            const u8 opcodeValue,
            const u8*& bytecodesStart,
            model::bytecode::Instruction& instruction
        ) -> void {
            const u8* switchStart = bytecodesStart;
            const u32 switchLen = computeSwitchLength(switchStart, opcodeValue);
            readSwitchData(instruction.switchOperandBytes, switchStart, switchLen);

            instruction.operand.type = model::bytecode::OperandType::Switch;
            instruction.operand.length = (u8) switchLen;
            instruction.operand.value = 0;
            instruction.length = 1 + switchLen;
            bytecodesStart += switchLen;
        }

        auto parseWidePrefix(
            const u8*& bytecodesStart,
            const u8* start,
            const u8* end,
            model::bytecode::Instruction& instruction
        ) -> bool {
            if (bytecodesStart >= end) {
                return false;
            }

            const u8 innerOpcodeValue = util::readU1(bytecodesStart);
            const auto* innerOpcode = &model::bytecode::OPCODE_TABLE[innerOpcodeValue];
            if (!innerOpcode || innerOpcode->mnemonic == nullptr) {
                return false;
            }

            auto wideTemplate = innerOpcode->operandTemplate;
            switch (innerOpcode->operandTemplate) {
                using namespace model::bytecode;
                case OperandTemplate::LocalVarIndex: {
                    wideTemplate = OperandTemplate::WideLocalVarIndex;
                    break;
                }
                case OperandTemplate::ConstantPoolIndex: {
                    wideTemplate = OperandTemplate::WideConstantPoolIndex;
                    break;
                }
                case OperandTemplate::BranchOffset: {
                    wideTemplate = OperandTemplate::WideBranchOffset;
                    break;
                }
                default: break;
            }

            instruction.opcode = innerOpcode;
            instruction.isWide = true;
            instruction.offset = (u32) (bytecodesStart - start - 2);
            parseOperandByTemplate(wideTemplate, bytecodesStart, instruction.operand);
            instruction.length = 1 + 1 + instruction.operand.length;

            return true;
        }

        auto parseInvokeDynamic(
            const model::bytecode::Opcode* opcode,
            const u8*& bytecodesStart,
            const u8* start,
            model::bytecode::Instruction& instruction
        ) -> void {
            const u16 index = util::readU2(bytecodesStart);
            const u16 reserved = util::readU2(bytecodesStart);

            instruction.opcode = opcode;
            instruction.offset = (u32) (bytecodesStart - start - 1);
            instruction.operand.type = model::bytecode::OperandType::Int;
            instruction.operand.length = 4;
            instruction.operand.value = index;
            instruction.operand.bytes[0] = (u8) (index >> 8 & 0xFF);
            instruction.operand.bytes[1] = (u8) (index & 0xFF);
            instruction.operand.bytes[2] = 0;
            instruction.operand.bytes[3] = 0;
            instruction.isWide = false;
            instruction.length = 1 + 4;
        }
    }

    auto BytecodesParser::parse(
        const u8* bytecodesStart,
        const usize bytecodesLen
    ) -> model::common::ElasticArray<model::bytecode::Instruction> {
        auto instructions = model::common::ElasticArray<model::bytecode::Instruction>();

        const u8* end = bytecodesStart + bytecodesLen;
        const u8* start = bytecodesStart;
        while (bytecodesStart < end) {
            const u8 opcodeValue = util::readU1(bytecodesStart);
            const auto* opcode = &model::bytecode::OPCODE_TABLE[opcodeValue];
            if (!opcode || opcode->mnemonic == nullptr) {
                continue;
            }

            auto instruction = model::bytecode::Instruction();

            if (opcodeValue == (u8) model::bytecode::Opcodes::wide) {
                if (!parseWidePrefix(bytecodesStart, start, end, instruction)) {
                    continue;
                }
                instructions.push(std::move(instruction));
                continue;
            }

            if (opcodeValue == (u8) model::bytecode::Opcodes::invokedynamic) {
                parseInvokeDynamic(opcode, bytecodesStart, start, instruction);
                instructions.push(std::move(instruction));
                continue;
            }

            instruction.opcode = opcode;
            instruction.offset = (u32) (bytecodesStart - start - 1);

            if (opcode->operandTemplate == model::bytecode::OperandTemplate::Switch) {
                parseSwitchInstruction(opcodeValue, bytecodesStart, instruction);
            } else {
                parseOperandByTemplate(opcode->operandTemplate, bytecodesStart, instruction.operand);
                instruction.length = 1 + instruction.operand.length;
            }

            instructions.push(std::move(instruction));
        }

        return instructions;
    }

    auto BytecodesParser::emit(
        const model::common::ElasticArray<model::bytecode::Instruction>& instructions
    ) -> model::common::ElasticArray<u8> {
        auto bytecode = model::common::ElasticArray<u8>();

        for (const auto& instruction : instructions) {
            if (instruction.isWide) {
                bytecode.push((u8) model::bytecode::Opcodes::wide);
            }

            bytecode.push(instruction.opcode->value);

            if (instruction.opcode->operandTemplate == model::bytecode::OperandTemplate::Switch) {
                for (auto switchOperandByte : instruction.switchOperandBytes) {
                    bytecode.push(switchOperandByte);
                }
            } else {
                const u8* operandBytes = instruction.getEffectiveOperandBytes();
                for (u8 i = 0; i < instruction.getEffectiveOperandLength(); ++i) {
                    bytecode.push(operandBytes[i]);
                }
            }
        }

        return bytecode;
    }
}
