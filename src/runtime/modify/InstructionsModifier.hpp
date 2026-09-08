#pragma once
#include "../../model/bytecode/Instruction.hpp"
#include "../../model/class/ClassMetadata.hpp"

namespace JByteDom::runtime::modify {
    class InstructionsModifier {
        public:
            explicit InstructionsModifier(
                model::clazz::ClassMetadata& classMetadata,
                model::clazz::MethodInfo& methodInfo
            );

            auto insertHead(const model::bytecode::Instruction& instruction) -> bool;
            auto insertHead(std::initializer_list<model::bytecode::Instruction> instructionList) -> bool;
            auto insertTail(const model::bytecode::Instruction& instruction) -> bool;
            auto insertTail(std::initializer_list<model::bytecode::Instruction> instructionList) -> bool;

            auto insertBefore(usize index, const model::bytecode::Instruction& instruction) -> bool;
            auto insertBefore(usize index, std::initializer_list<model::bytecode::Instruction> instructionList) -> bool;
            auto insertAfter(usize index, const model::bytecode::Instruction& instruction) -> bool;
            auto insertAfter(usize index, std::initializer_list<model::bytecode::Instruction> instructionList) -> bool;

            auto removeHead() -> bool;
            auto removeTail() -> bool;
            auto removeAt(usize index) -> bool;

            auto makeLdcString(const char* str) const -> model::bytecode::Instruction;
            auto makeGetStatic(const char* className, const char* fieldName, const char* descriptor) const -> model::bytecode::Instruction;
            auto makePutStatic(const char* className, const char* fieldName, const char* descriptor) const -> model::bytecode::Instruction;
            auto makeInvokeVirtual(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction;
            auto makeInvokeSpecial(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction;
            auto makeInvokeStatic(const char* className, const char* methodName, const char* descriptor) const -> model::bytecode::Instruction;

            auto insertPrintlnHead(const char* str) -> bool;
            auto insertPrintlnTail(const char* str) -> bool;
            auto insertPrintlnBefore(usize index, const char* str) -> bool;
            auto insertPrintlnAfter(usize index, const char* str) -> bool;


            auto isModified() const -> bool { return modified; }

            auto applyAllModification() -> bool;


        private:
            model::clazz::ClassMetadata& classMetadata;
            model::clazz::MethodInfo& methodInfo;

            u16 maxStack;
            u16 maxLocals;
            model::common::ElasticArray<model::bytecode::Instruction> instructions;

            bool modified = false;
    };
}
