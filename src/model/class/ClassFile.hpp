#pragma once
#include "../../CommonPrimitives.hpp"

namespace JByteDom::model::clazz {
    class ClassFile {
        public:
            ClassFile(
                const char* inPath,
                u64 inBytesLen,
                u8* inBytes
            );
            ~ClassFile();

            ClassFile(const ClassFile& other);
            ClassFile& operator = (const ClassFile& other);

            ClassFile(ClassFile&& other) noexcept;
            ClassFile& operator = (ClassFile&& other) noexcept;

            auto getPath() const -> const char* { return this->path; }
            auto getBytesLen() const -> u64 { return this->bytesLen; }
            auto getBytes() const -> const u8* { return this->bytes; }


        private:
            char* path = nullptr;
            u64 bytesLen = 0;
            u8* bytes = nullptr;
    };
}