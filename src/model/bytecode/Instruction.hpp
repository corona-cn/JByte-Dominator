#pragma once
#include "Opcode.hpp"
#include "Operand.hpp"
#include "../class/ClassMetadata.hpp"
#include "../common/ElasticArray.hpp"

namespace JByteDom::model::bytecode {
    class Instruction {
        public:
            Instruction() = default;
            ~Instruction();

            Instruction(const Instruction& other);
            Instruction& operator = (const Instruction& other);

            Instruction(Instruction&& other) noexcept;
            Instruction& operator = (Instruction&& other) noexcept;

            const Opcode* opcode = nullptr;
            Operand operand;
            u32 offset = 0;
            u32 length = 0;
            bool isWide = false;

            common::ElasticArray<u8> switchOperandBytes;

            auto isModified() const -> bool { return this->modifyIndicator; }

            auto modifyOperand(
                const u8* newData,
                u8 newLen
            ) -> void;

            auto getEffectiveOperandBytes() const -> const u8*;
            auto getEffectiveOperandLength() const -> u8;

            auto getDisplayFullName(
                const clazz::ClassMetadata& classMetadata
            ) const -> const char*;

            static auto make(Opcodes opcodeType) -> Instruction;
            static auto make(Opcodes opcodeType, u8 operand) -> Instruction;
            static auto make(Opcodes opcodeType, u16 operand) -> Instruction;
            static auto make(Opcodes opcodeType, u32 operand) -> Instruction;
            static auto make(
                Opcodes opcodeType,
                const u8* operands,
                u8 operandsLen
            ) -> Instruction;


        private:
            bool modifyIndicator = false;
            common::ElasticArray<u8> modifiedOperands;
    };
}
