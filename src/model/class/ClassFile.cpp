#include "ClassFile.hpp"

#include <cstdlib>
#include <cstring>

namespace JByteDom::model::clazz {
    ClassFile::ClassFile(
        const char* inPath,
        const u64 inBytesLen,
        u8* inBytes
    ):
        bytesLen(inBytesLen),
        bytes(inBytes)
    {
        if (inPath) {
            this->path = (char*) malloc(strlen(inPath) + 1);
            if (this->path) {
                strcpy(this->path, inPath);
            }
        }
    }
    ClassFile::~ClassFile() {
        free(path);
        free(bytes);
    }

    ClassFile::ClassFile(const ClassFile& other):
        bytesLen(other.bytesLen)
    {
        if (other.path) {
            this->path = (char*) malloc(strlen(other.path) + 1);
            if (this->path) {
                strcpy(this->path, other.path);
            }
        }

        if (other.bytes && other.bytesLen > 0) {
            this->bytes = (u8*) malloc(other.bytesLen);
            if (this->bytes) {
                memcpy(this->bytes, other.bytes, other.bytesLen);
            }
        }
    }
    ClassFile& ClassFile::operator = (const ClassFile& other) {
        if (this != &other) {
            free(this->path);
            free(this->bytes);

            this->path = nullptr;
            this->bytesLen = other.bytesLen;
            this->bytes = nullptr;

            if (other.path) {
                this->path = (char*) malloc(strlen(other.path) + 1);
                if (this->path) {
                    strcpy(this->path, other.path);
                }
            }

            if (other.bytes && other.bytesLen > 0) {
                this->bytes = (u8*) malloc(other.bytesLen);
                if (this->bytes) {
                    memcpy(this->bytes, other.bytes, other.bytesLen);
                }
            }
        }
        return *this;
    }

    ClassFile::ClassFile(ClassFile&& other) noexcept:
        path(other.path),
        bytesLen(other.bytesLen),
        bytes(other.bytes)
    {
        other.path = nullptr;
        other.bytesLen = 0;
        other.bytes = nullptr;
    }
    ClassFile& ClassFile::operator = (ClassFile&& other) noexcept {
        if (this != &other) {
            free(this->path);
            free(this->bytes);

            this->path = other.path;
            this->bytesLen = other.bytesLen;
            this->bytes = other.bytes;

            other.path = nullptr;
            other.bytesLen = 0;
            other.bytes = nullptr;
        }

        return *this;
    }
}
