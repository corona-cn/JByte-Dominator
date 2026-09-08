#include "FileUtils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "StringUtils.hpp"

namespace JByteDom::util {
    namespace {
        auto fileModeToStr(
            const FileMode fileMode
        ) -> const char* {
            switch (fileMode) {
                case Read: return "r";
                case Write: return "w";
                case ReadBinary: return "rb";
                case WriteBinary: return "wb";
                default: return "r";
            }
        }

        #ifdef _WIN32
        auto fileModeToWideStr(
            const FileMode fileMode
        ) -> const wchar* {
            switch (fileMode) {
                case Read: return L"r";
                case Write: return L"w";
                case ReadBinary: return L"rb";
                case WriteBinary: return L"wb";
                default: return L"r";
            }
        }
        #endif
    }

    auto openFileUtf8(
        const char* utf8FilePath,
        const FileMode fileMode
    ) -> FILE* {
        #ifdef _WIN32
            wchar* wideFilePath = widenUtf8Dup(utf8FilePath);
            if (!wideFilePath) {
                return nullptr;
            }

            const wchar* fileModeWideStr = fileModeToWideStr(fileMode);
            FILE* file = _wfopen(wideFilePath, fileModeWideStr);
            free(wideFilePath);

            return file;
        #else
            const char* fileModeStr = fileModeToStr(fileMode);
            return fopen(utf8FilePath, fileModeStr);
        #endif
    }

    auto parseFileNameDup(
        const char* filePath
    ) -> char* {
        if (!filePath || *filePath == '\0') {
            return makeEmptyStr();
        }

        const char* lastSep = nullptr;
        for (const char* ptr = filePath; *ptr; ++ptr) {
            if (*ptr == '/' || *ptr == '\\') {
                lastSep = ptr;
            }
        }

        const char* nameStart = lastSep ? lastSep + 1 : filePath;
        if (*nameStart == '\0') {
            return makeEmptyStr();
        }

        const usize nameLen = strlen(nameStart);
        const auto result = (char*) malloc(nameLen + 1);
        if (!result) {
            return nullptr;
        }

        strcpy(result, nameStart);

        return result;
    }

    auto parseFileExtensionDup(
        const char* filePath
    ) -> char* {
        if (!filePath || *filePath == '\0') {
            return makeEmptyStr();
        }

        const auto fileName = parseFileNameDup(filePath);
        if (!fileName) {
            return makeEmptyStr();
        }

        if (*fileName == '\0') {
            free(fileName);
            return makeEmptyStr();
        }

        const char* lastDot = nullptr;
        for (const char* ptr = fileName; *ptr; ++ptr) {
            if (*ptr == '.') {
                lastDot = ptr;
            }
        }

        if (!lastDot || *(lastDot + 1) == '\0') {
            free(fileName);
            return makeEmptyStr();
        }

        const char* extensionStart = lastDot + 1;
        const usize extensionLen = strlen(extensionStart);
        const auto result = (char*) malloc(extensionLen + 1);
        if (!result) {
            free(fileName);
            return nullptr;
        }

        strcpy(result, extensionStart);

        free(fileName);

        return result;
    }
}
