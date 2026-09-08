#pragma once
#include <cstdio>

namespace JByteDom::util {
    enum FileMode {
        Read,
        Write,
        ReadBinary,
        WriteBinary,
    };

    auto openFileUtf8(
        const char* utf8FilePath,
        FileMode fileMode
    ) -> FILE*;

    auto parseFileNameDup(
        const char* filePath
    ) -> char*;

    auto parseFileExtensionDup(
        const char* filePath
    ) -> char*;
}
