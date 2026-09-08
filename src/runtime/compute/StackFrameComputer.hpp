#pragma once
#include "../../CommonPrimitives.hpp"
#include "../../model/bytecode/Instruction.hpp"
#include "../../model/common/ElasticArray.hpp"

namespace JByteDom::runtime::compute {
    struct FrameInfo {
        u16 maxStack;
        u16 maxLocals;
    };

    class StackFrameComputer {
        public:
            static auto computeFrameInfo(
                const model::common::ElasticArray<model::bytecode::Instruction>& instructions
            ) -> FrameInfo;
    };
}
