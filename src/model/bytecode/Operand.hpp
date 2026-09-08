#pragma once
#include "../../CommonPrimitives.hpp"

namespace JByteDom::model::bytecode {
    enum class OperandType : u8 {
        None,
        Byte,
        Short,
        Int,
        Long,
        WideShort,
        Switch,
        MultiANewArray
    };

    enum class OperandTemplate : u8 {
        None,
        ByteImm,
        ShortImm,
        LocalVarIndex,
        WideLocalVarIndex,
        ConstantPoolIndex,
        WideConstantPoolIndex,
        BranchOffset,
        WideBranchOffset,
        Switch,
        MultiANewArray
    };

    struct Operand {
        OperandType type;
        u8 length;
        u8 bytes[8];
        u64 value;
    };
}
