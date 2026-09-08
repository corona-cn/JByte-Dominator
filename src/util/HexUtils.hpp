#pragma once
#include "../CommonPrimitives.hpp"
#include "../CommonStruct.hpp"

namespace JByteDom::util {
    auto encode(
        Span<const u8> bytesSpan,
        bool spaced = true
    ) -> char*;

    auto encode(
        Span<const u8> bytesSpan,
        usize maxBytesLen,
        bool spaced = true
    ) -> char*;

    auto decode(
        const char* hexStr
    ) -> Span<u8>;
}
