#include "JarEntry.hpp"

#include <cstring>
#include <ostream>
#include <zip.h>

#include "../../util/FileUtils.hpp"

namespace JByteDom::model::jar {
    namespace {
        auto guessEntryType(const char* entryPath) -> EntryType {
            if (!entryPath || *entryPath == '\0') {
                return EntryType::Unknown;
            }

            const usize pathLen = strlen(entryPath);
            if (pathLen > 0 && entryPath[pathLen - 1] == '/') {
                return EntryType::Directory;
            }

            return EntryType::File;
        }
    }

    JarEntry::JarEntry(
        const char* inPath,
        const u64 inBytesLen,
        const zip_int64_t inIndex,
        const u64 inCompressedSize,
        const u64 inUncompressedSize,
        const CompressionMethod inCompressionMethod
    ):
        name(util::parseFileNameDup(inPath)),
        extension(util::parseFileExtensionDup(inPath)),
        bytesLen(inBytesLen),
        index(inIndex),
        compressedSize(inCompressedSize),
        uncompressedSize(inUncompressedSize),
        compressionMethod(inCompressionMethod),
        entryType(guessEntryType(inPath))
    {
        if (inPath) {
            this->path = (char*) malloc(strlen(inPath) + 1);
            if (this->path) {
                strcpy(this->path, inPath);
            }
        }
    }
    JarEntry::~JarEntry() {
        free(this->path);
        free(this->name);
        free(this->extension);
    }

    JarEntry::JarEntry(JarEntry&& other) noexcept:
        path(other.path),
        name(other.name),
        extension(other.extension),
        bytesLen(other.bytesLen),
        index(other.index),
        compressedSize(other.compressedSize),
        uncompressedSize(other.uncompressedSize),
        compressionMethod(other.compressionMethod),
        entryType(other.entryType)
    {
        other.path = nullptr;
        other.name = nullptr;
        other.extension = nullptr;
        other.bytesLen = 0;
        other.index = 0;
        other.compressedSize = 0;
        other.uncompressedSize = 0;
        other.compressionMethod = CompressionMethod::Unknown;
        other.entryType = EntryType::Unknown;
        other.error = nullptr;
    }
    JarEntry& JarEntry::operator = (JarEntry&& other) noexcept {
        if (this != &other) {
            free(this->path);
            free(this->name);
            free(this->extension);

            this->path = other.path;
            this->name = other.name;
            this->extension = other.extension;
            this->bytesLen = other.bytesLen;
            this->index = other.index;
            this->compressedSize = other.compressedSize;
            this->uncompressedSize = other.uncompressedSize;
            this->compressionMethod = other.compressionMethod;
            this->entryType = other.entryType;
            this->error = other.error;

            other.path = nullptr;
            other.name = nullptr;
            other.extension = nullptr;
            other.bytesLen = 0;
            other.index = 0;
            other.compressedSize = 0;
            other.uncompressedSize = 0;
            other.compressionMethod = CompressionMethod::Unknown;
            other.entryType = EntryType::Unknown;
            other.error = nullptr;
        }

        return *this;
    }

    auto JarEntry::compressionMethodToStr() const -> const char* {
        switch (this->compressionMethod) {
            case CompressionMethod::Store: return "Store";
            case CompressionMethod::Deflate: return "Deflate";
            case CompressionMethod::Deflate64: return "Deflate64";
            case CompressionMethod::BZip2: return "BZip2";
            case CompressionMethod::LZMA: return "LZMA";
            case CompressionMethod::Zstd: return "Zstd";
            case CompressionMethod::MP3: return "MP3";
            case CompressionMethod::XZ: return "XZ";
            case CompressionMethod::JPEG: return "JPEG";
            case CompressionMethod::WavPack: return "WavPack";
            case CompressionMethod::PPMd: return "PPMd";
            default: return "Unknown";
        }
    }
    auto JarEntry::entryTypeToStr() const -> const char* {
        switch (entryType) {
            case EntryType::File: return "File";
            case EntryType::Directory: return "Directory";
            case EntryType::Symlink: return "Symlink";
            default: return "Unknown";
        }
    }

    auto JarEntry::extractBytes(JarFile& jarFile) -> u8* {
        // 清空字段
        this->error = nullptr;

        if (this->uncompressedSize == 0) {
            return nullptr;
        }

        zip_t* zipHandle = jarFile.getZipHandle();
        if (!zipHandle) {
            this->error = "获取 ZIP 句柄失败";
            return nullptr;
        }

        // 尝试打开 ZIP 条目
        zip_file_t* file = zip_fopen_index(zipHandle, index, 0);
        if (!file) {
            this->error = "打开 ZIP 条目失败";
            return nullptr;
        }

        // 尝试为条目二进制数据分配内存
        const auto result = (u8*) malloc(uncompressedSize);
        if (!result) {
            zip_fclose(file);
            this->error = "条目二进制数据变量内存分配失败";
            return nullptr;
        }

        // 尝试读取条目二进制数据
        const zip_int64_t bytes_read = zip_fread(file, result, uncompressedSize);
        if (bytes_read != uncompressedSize) {
            free(result);
            zip_fclose(file);
            this->error = "读取条目二进制数据失败";
            return nullptr;
        }

        // 关闭文件
        zip_fclose(file);

        return result;
    }
}
