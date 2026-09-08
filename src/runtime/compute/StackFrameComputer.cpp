#include "StackFrameComputer.hpp"
#include "../../model/bytecode/Opcode.hpp"

namespace JByteDom::runtime::compute {
    auto StackFrameComputer::computeFrameInfo(
        const model::common::ElasticArray<model::bytecode::Instruction>& instructions
    ) -> FrameInfo {
        u16 maxStack = 0;
        u16 maxLocals = 0;
        i32 currentStack = 0;

        for (usize i = 0; i < instructions.getSize(); ++i) {
            const auto& instruction = instructions[i];
            const auto* opcode = instruction.opcode;
            if (!opcode) {
                continue;
            }

            currentStack += opcode->stackDelta;
            if (currentStack < 0) {
                currentStack = 0;
            }

            if ((u16) currentStack > maxStack) {
                maxStack = (u16) currentStack;
            }

            if (opcode->referencesLocal) {
                const u16 index = (u16) instruction.operand.value;
                const u16 needed = index + (opcode->isWideLocal ? 2 : 1);
                if (needed > maxLocals) {
                    maxLocals = needed;
                }
            }
        }

        return {maxStack, maxLocals};
    }
}
