#pragma once
#include "../../CommonPrimitives.hpp"
#include "../../model/bytecode/Instruction.hpp"
#include "../../model/common/ElasticArray.hpp"

namespace JByteDom::preprocess::parse {
    class BytecodesParser {
        public:
            static auto parse(
                const u8* bytecodesStart,
                usize bytecodesLen
            ) -> model::common::ElasticArray<model::bytecode::Instruction>;

            static auto emit(
                const model::common::ElasticArray<model::bytecode::Instruction>& instructions
            )-> model::common::ElasticArray<u8>;
    };
}
