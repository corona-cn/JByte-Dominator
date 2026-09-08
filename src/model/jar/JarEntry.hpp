#pragma once
#include <zipconf.h>

#include "JarFile.hpp"
#include "../../CommonPrimitives.hpp"

namespace JByteDom::model::jar {
    enum class CompressionMethod : u16 {
        Store = 0,
        Deflate = 8,
        Deflate64 = 9,
        BZip2 = 12,
        LZMA = 14,
        Zstd = 93,
        MP3 = 94,
        XZ = 95,
        JPEG = 96,
        WavPack = 97,
        PPMd = 98,
        Unknown = 0xFFFF,
    };

    enum class EntryType : u8 {
        File,
        Directory,
        Symlink,
        Unknown,
    };

    class JarEntry {
        public:
            explicit JarEntry(
                const char* inPath,
                u64 inBytesLen,
                zip_int64_t inIndex,
                u64 inCompressedSize,
                u64 inUncompressedSize,
                CompressionMethod inCompressionMethod
            );
            ~JarEntry();

            JarEntry(const JarEntry&) = delete;
            JarEntry& operator = (const JarEntry&) = delete;

            JarEntry(JarEntry&& other) noexcept;
            JarEntry& operator = (JarEntry&& other) noexcept;

            auto getPath() const -> const char* { return this->path; }
            auto getName() const -> const char* { return this->name; }
            auto getExtension() const -> const char* { return this->extension; }
            auto getBytesLen() const -> u64 { return this->bytesLen; }
            auto getIndex() const -> zip_int64_t { return this->index; }
            auto getCompressedSize() const -> u64 { return this->compressedSize; }
            auto getUncompressedSize() const -> u64 { return this->uncompressedSize; }
            auto getCompressionMethod() const -> CompressionMethod { return this->compressionMethod; }
            auto compressionMethodToStr() const -> const char*;
            auto getEntryType() const -> EntryType { return this->entryType; }
            auto entryTypeToStr() const -> const char*;
            auto getError() const -> const char* { return this->error; }

            auto extractBytes(JarFile& jarFile) -> u8*;


        private:
            char* path = nullptr;
            char* name = nullptr;
            char* extension = nullptr;
            u64 bytesLen = 0;
            zip_int64_t index = 0;
            u64 compressedSize = 0;
            u64 uncompressedSize = 0;
            CompressionMethod compressionMethod = CompressionMethod::Unknown;
            EntryType entryType = EntryType::Unknown;
            const char* error = nullptr;
    };
}
