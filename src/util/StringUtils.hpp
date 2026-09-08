#pragma once
#include "../CommonPrimitives.hpp"

namespace JByteDom::util {
    auto makeStr(
        const char* firstStr,
        ...
    ) -> char*;

    auto makeEmptyStr() -> char*;

    #ifdef _WIN32
    auto widenUtf8Dup(
        const char* utf8Str
    ) -> wchar*;
    #endif

    auto replaceAllCharsDup(
        const char* str,
        char oldChar,
        char newChar
    ) -> char*;
}

#define MAKE_STR(...) \
    JByteDom::util::makeStr(__VA_ARGS__, nullptr)