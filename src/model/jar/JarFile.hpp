#pragma once
#include <zip.h>

#include "../../CommonPrimitives.hpp"

namespace JByteDom::model::jar {
    class JarFile {
        public:
            explicit JarFile(const char* inPath);
            ~JarFile();

            JarFile(const JarFile&) = delete;
            JarFile& operator = (const JarFile&) = delete;

            JarFile(JarFile&& other) noexcept;
            JarFile& operator = (JarFile&& other) noexcept;

            auto getPath() const -> const char* { return this->path; }
            auto getLen() const -> usize { return this->len; }
            auto getBytes() const -> const u8* { return this->bytes; }
            auto getError() const -> const char* { return this->error; }

            auto openZip() -> bool;
            auto closeZip() -> void;
            auto getZipHandle() -> zip_t*;

        private:
            char* path = nullptr;
            usize len = 0;
            u8* bytes = nullptr;
            const char* error = nullptr;
            zip_t* zipHandle = nullptr;
    };
}