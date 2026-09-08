#pragma once
#include "../CommonPrimitives.hpp"
#include "../model/common/ElasticArray.hpp"

namespace JByteDom::util {
    auto readU1(const u8*& bytesIn) -> u8;
    auto readU2(const u8*& bytesIn) -> u16;
    auto readU4(const u8*& bytesIn) -> u32;
    auto readU8(const u8*& bytesIn) -> u64;

    auto readU1(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u8;
    auto readU2(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u16;
    auto readU4(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u32;
    auto readU8(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u64;

    auto writeU1(u8*& bytesOut, u8 value) -> void;
    auto writeU2(u8*& bytesOut, u16 value) -> void;
    auto writeU4(u8*& bytesOut, u32 value) -> void;
    auto writeU8(u8*& bytesOut, u64 value) -> void;

    auto writeU1(model::common::ElasticArray<u8>& bytesOut, u8 value) -> void;
    auto writeU2(model::common::ElasticArray<u8>& bytesOut, u16 value) -> void;
    auto writeU4(model::common::ElasticArray<u8>& bytesOut, u32 value) -> void;
    auto writeU8(model::common::ElasticArray<u8>& bytesOut, u64 value) -> void;

    auto skipBytes(const u8*& bytesIn, usize bytesLen) -> void;

    auto skipBytes(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos, usize bytesLen) -> void;
}
