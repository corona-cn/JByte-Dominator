#include "ByteUtils.hpp"

namespace JByteDom::util {
    auto readU1(const u8*& bytesIn) -> u8 {
        return *bytesIn++;
    }
    auto readU2(const u8*& bytesIn) -> u16 {
        const u16 value = (((u16) *(bytesIn + 0)) << 8) | ((u16) *(bytesIn + 1));
        bytesIn += 2;
        return value;
    }
    auto readU4(const u8*& bytesIn) -> u32 {
        const u32 value = (((u32) *(bytesIn + 0)) << 24) | (((u32) *(bytesIn + 1)) << 16) | (((u32) *(bytesIn + 2)) << 8) | ((u32) *(bytesIn + 3));
        bytesIn += 4;
        return value;
    }
    auto readU8(const u8*& bytesIn) -> u64 {
        const u64 upper = ((u64) readU4(bytesIn)) << 32;
        const u64 lower  = readU4(bytesIn);
        return upper | lower;
    }

    auto readU1(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u8 {
        return bytesIn[bytesPos++];
    }
    auto readU2(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u16 {
        const u16 value = (((u16) bytesIn[bytesPos]) << 8) | ((u16) bytesIn[bytesPos + 1]);
        bytesPos += 2;
        return value;
    }
    auto readU4(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u32 {
        const u32 value = (((u32) bytesIn[bytesPos]) << 24) | (((u32) bytesIn[bytesPos + 1]) << 16) | (((u32) bytesIn[bytesPos + 2]) << 8) | ((u32) bytesIn[bytesPos + 3]);
        bytesPos += 4;
        return value;
    }
    auto readU8(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos) -> u64 {
        const u64 upper = ((u64) readU4(bytesIn, bytesPos)) << 32;
        const u64 lower  = readU4(bytesIn, bytesPos);
        return upper | lower;
    }

    auto writeU1(u8*& bytesOut, const u8 value) -> void {
        *bytesOut++ = value;
    }
    auto writeU2(u8*& bytesOut, const u16 value) -> void {
        *(bytesOut + 0) = (u8) ((value >> 8) & 0xFF);
        *(bytesOut + 1) = (u8) (value & 0xFF);
        bytesOut += 2;
    }
    auto writeU4(u8*& bytesOut, const u32 value) -> void {
        *(bytesOut + 0) = (u8) ((value >> 24) & 0xFF);
        *(bytesOut + 1) = (u8) ((value >> 16) & 0xFF);
        *(bytesOut + 2) = (u8) ((value >> 8) & 0xFF);
        *(bytesOut + 3) = (u8) (value & 0xFF);
        bytesOut += 4;
    }
    auto writeU8(u8*& bytesOut, const u64 value) -> void {
        const u32 high = (u32) ((value >> 32) & 0xFFFFFFFF);
        const u32 low = (u32) (value & 0xFFFFFFFF);
        writeU4(bytesOut, high);
        writeU4(bytesOut, low);
    }

    auto writeU1(model::common::ElasticArray<u8>& bytesOut, const u8 value) -> void {
        bytesOut.push(value);
    }
    auto writeU2(model::common::ElasticArray<u8>& bytesOut, const u16 value) -> void {
        bytesOut.push((u8) ((value >> 8) & 0xFF));
        bytesOut.push((u8) (value & 0xFF));
    }
    auto writeU4(model::common::ElasticArray<u8>& bytesOut, const u32 value) -> void {
        bytesOut.push((u8) ((value >> 24) & 0xFF));
        bytesOut.push((u8) ((value >> 16) & 0xFF));
        bytesOut.push((u8) ((value >> 8) & 0xFF));
        bytesOut.push((u8) (value & 0xFF));
    }
    auto writeU8(model::common::ElasticArray<u8>& bytesOut, const u64 value) -> void {
        const u32 high = (u32) ((value >> 32) & 0xFFFFFFFF);
        const u32 low = (u32) (value & 0xFFFFFFFF);
        writeU4(bytesOut, high);
        writeU4(bytesOut, low);
    }

    auto skipBytes(const u8*& bytesIn, const usize bytesLen) -> void {
        bytesIn += bytesLen;
    }

    auto skipBytes(const model::common::ElasticArray<u8>& bytesIn, usize& bytesPos, const usize bytesLen) -> void {
        bytesPos += bytesLen;
    }
}
