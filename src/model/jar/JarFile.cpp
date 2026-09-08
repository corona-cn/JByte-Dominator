#include "JarFile.hpp"

#include <cstdio>
#include <cstring>
#include <zip.h>

#include "../../CommonMacro.hpp"
#include "../../util/FileUtils.hpp"

namespace JByteDom::model::jar {
    namespace {
        constexpr u32 ZIP_MAGIC = 0x04034b50;
    }

    JarFile::JarFile(const char* inPath) {
        // 清空字段
        this->path = nullptr;
        this->bytes = nullptr;
        this->len = 0;
        this->error = nullptr;
        closeZip();

        // 路径参数判空
        if (!inPath) {
            this->error = "路径参数为空";
            return;
        }

        // 尝试分配路径字段内存
        const usize pathLen = strlen(inPath) + 1;
        this->path = (char*) malloc(pathLen);
        if (!this->path) {
            PTR_FREE_AND_NULL(this->path);
            this->error = "路径字段内存分配失败";
            return;
        }

        // 复制路径参数内存给路径字段
        memcpy(this->path, inPath, pathLen);

        // 尝试打开文件
        FILE* file = util::openFileUtf8(this->path, util::FileMode::ReadBinary);
        if (!file) {
            PTR_FREE_AND_NULL(this->path);
            this->error = "文件打开失败";
            return;
        }

        // 尝试将文件位置指针移动到文件尾
        if (fseek(file, 0, SEEK_END) != 0) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            this->error = "文件位置指针移动失败： SEEK_END";
            return;
        }

        // 尝试读取文件长度
        const long fileLen = ftell(file);
        if (fileLen == -1L) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            this->error = "文件长度读取失败";
            return;
        }

        // 拷贝文件长度变量到文件长度参数
        this->len = (usize) fileLen;

        // 尝试将文件位置指针移动到文件头
        if (fseek(file, 0, SEEK_SET) != 0) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            this->error = "文件位置指针移动失败： SEEK_SET";
            return;
        }

        // 尝试分配文件二进制字段内存
        this->bytes = (u8*) malloc(this->len);
        if (!this->bytes) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            this->error = "文件二进制字段内存分配失败";
            return;
        }

        // 尝试读取文件二进制
        const usize readCount = fread(this->bytes, 1, this->len, file);
        if (readCount != this->len) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            PTR_FREE_AND_NULL(this->bytes);
            this->error = "文件内容读取失败";
            return;
        }

        // 检查文件长度是否小于文件魔数长度
        if (this->len < 4) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            PTR_FREE_AND_NULL(this->bytes);
            this->error = "文件过小";
            return;
        }

        // 验证文件魔数是否匹配
        const u32 fileMagic = ((u32) *(this->bytes + 0)) | (((u32) *(this->bytes + 1)) << 8) | (((u32) *(this->bytes + 2)) << 16) | (((u32) *(this->bytes + 3)) << 24);
        if (fileMagic != ZIP_MAGIC) {
            FILE_CLOSE_AND_NULL(file);
            PTR_FREE_AND_NULL(this->path);
            PTR_FREE_AND_NULL(this->bytes);
            this->error = "无效的 ZIP/JAR 魔数";
            return;
        }

        // 关闭文件
        fclose(file);
    }
    JarFile::~JarFile() {
        PTR_FREE_AND_NULL(this->path);
        PTR_FREE_AND_NULL(this->bytes);
        closeZip();
    }

    JarFile::JarFile(JarFile&& other) noexcept:
        path(other.path),
        len(other.len),
        bytes(other.bytes),
        error(other.error)
    {
        other.path = nullptr;
        other.len = 0;
        other.bytes = nullptr;
        other.error = nullptr;
        other.zipHandle = nullptr;
    }
    JarFile& JarFile::operator = (JarFile&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        free(this->path);
        free(this->bytes);
        closeZip();

        this->path = other.path;
        this->len = other.len;
        this->bytes = other.bytes;
        this->error = other.error;
        this->zipHandle = other.zipHandle;

        other.path = nullptr;
        other.len = 0;
        other.bytes = nullptr;
        other.error = nullptr;
        other.zipHandle = nullptr;

        return *this;
    }

    auto JarFile::openZip() -> bool {
        if (zipHandle) {
            return true;
        }

        if (!bytes || len == 0) {
            error = "JarFile 数据为空，无法打开 ZIP";
            return false;
        }

        auto zipError = zip_error_t{};
        zip_error_init(&zipError);
        zip_source_t* source = zip_source_buffer_create(this->bytes, this->len, 0, &zipError);
        if (!source) {
            this->error = "创建 ZIP 数据源失败";
            return false;
        }

        this->zipHandle = zip_open_from_source(source, ZIP_RDONLY, &zipError);
        if (!zipHandle) {
            this->error = "打开 ZIP 归档失败";
            zip_source_free(source);
            return false;
        }

        this->error = nullptr;

        return true;
    }
    auto JarFile::closeZip() -> void {
        if (this->zipHandle) {
            zip_close(zipHandle);
            this->zipHandle = nullptr;
        }
    }
    auto JarFile::getZipHandle() -> zip_t* {
        if (!openZip()) {
            return nullptr;
        }

        return zipHandle;
    }
}
