#include "HexUtils.hpp"

#include <cctype>
#include <cstdlib>

namespace JByteDom::util {
    namespace {
        constexpr char HEX_CHARS[] = "0123456789ABCDEF";
    }

    namespace {
        auto hexCharToNibble(const char hexChar) -> int {
            if (hexChar >= '0' && hexChar <= '9') {
                return hexChar - '0';
            }

            if (hexChar >= 'A' && hexChar <= 'F') {
                return hexChar - 'A' + 10;
            }

            if (hexChar >= 'a' && hexChar <= 'f') {
                return hexChar - 'a' + 10;
            }

            return -1;
        }
    }

    auto encode(
        const Span<const u8> bytesSpan,
        const bool spaced
    ) -> char* {
        // 字节数据流异常
        if (!bytesSpan.ptr || bytesSpan.len == 0) {
            const auto emptyStr = (char*) malloc(1);
            if (!emptyStr) {
                return nullptr;
            }

            *emptyStr = '\0';

            return emptyStr;
        }

        // 分配结果内存
        const auto charsPerByte = spaced ? 3 : 2;
        const usize bytesSize = bytesSpan.len * charsPerByte;
        const auto result = (char*) malloc(bytesSize + 1);
        if (!result) {
            return nullptr;
        }

        char* writePtr = result;

        // 逐字节遍历，映射：Nibble -> HexChar
        if (spaced) {
            for (usize i = 0; i < bytesSpan.len; ++i) {
                const u8 currentByte = bytesSpan.ptr[i];

                // 取高低半字节位
                const u8 upperNibble = (currentByte >> 4) & 0x0F;
                const u8 lowerNibble = currentByte & 0x0F;

                // 查表映射为高低位十六进制字符
                const char upperHexChar = HEX_CHARS[upperNibble];
                const char lowerHexChar = HEX_CHARS[lowerNibble];

                *writePtr++ = upperHexChar;
                *writePtr++ = lowerHexChar;
                *writePtr++ = ' ';
            }

            // 遍历后退位，为结束字符留出空间
            writePtr--;
        } else {
            for (usize i = 0; i < bytesSpan.len; ++i) {
                const u8 currentByte = bytesSpan.ptr[i];

                // 取高低半字节位
                const u8 upperNibble = (currentByte >> 4) & 0x0F;
                const u8 lowerNibble = currentByte & 0x0F;

                // 查表映射为高低位十六进制字符
                const char upperHexChar = HEX_CHARS[upperNibble];
                const char lowerHexChar = HEX_CHARS[lowerNibble];

                *writePtr++ = upperHexChar;
                *writePtr++ = lowerHexChar;
            }
        }

        // 补上结束字符
        *writePtr = '\0';

        return result;
    }

    auto encode(
        const Span<const u8> bytesSpan,
        const usize maxBytesLen,
        const bool spaced
    ) -> char* {
        // 字节数据流异常或参数异常
        if (!bytesSpan.ptr || bytesSpan.len == 0 || maxBytesLen == 0) {
            const auto emptyStr = (char*) malloc(1);
            if (!emptyStr) {
                return nullptr;
            }

            *emptyStr = '\0';

            return emptyStr;
        }

        // 计算有效最大字节长度
        const usize validMaxBytesLen = (maxBytesLen < bytesSpan.len) ? maxBytesLen : bytesSpan.len;

        // 分配结果内存
        const uint charsPerByte = spaced ? 3 : 2;
        const usize bytesSize = validMaxBytesLen * charsPerByte;
        const auto result = (char*) malloc(bytesSize + 1);
        if (!result) {
            return nullptr;
        }

        char* writePtr = result;

        // 逐字节遍历，映射：Nibble -> HexChar
        if (spaced) {
            for (usize i = 0; i < validMaxBytesLen; ++i) {
                const u8 currentByte = bytesSpan.ptr[i];

                // 取高低半字节位
                const u8 upperNibble = (currentByte >> 4) & 0x0F;
                const u8 lowerNibble = currentByte & 0x0F;

                // 查表映射为高低位十六进制字符
                const char upperHexChar = HEX_CHARS[upperNibble];
                const char lowerHexChar = HEX_CHARS[lowerNibble];

                *writePtr++ = upperHexChar;
                *writePtr++ = lowerHexChar;
                *writePtr++ = ' ';
            }

            // 遍历后退位，为结束字符留出空间
            writePtr--;
        } else {
            for (usize i = 0; i < validMaxBytesLen; ++i) {
                const u8 currentByte = bytesSpan.ptr[i];

                // 取高低半字节位
                const u8 upperNibble = (currentByte >> 4) & 0x0F;
                const u8 lowerNibble = currentByte & 0x0F;

                // 查表映射为高低位十六进制字符
                const char upperHexChar = HEX_CHARS[upperNibble];
                const char lowerHexChar = HEX_CHARS[lowerNibble];

                *writePtr++ = upperHexChar;
                *writePtr++ = lowerHexChar;
            }
        }

        // 补上结束字符
        *writePtr = '\0';

        return result;
    }

    auto decode(
        const char* hexStr
    ) -> Span<u8> {
        if (!hexStr) {
            return {nullptr, 0};
        }

        // 传入空字符串，直接输出空字节数据流
        if (hexStr[0] == '\0') {
            const auto empty = Span<u8>{(u8*) malloc(1), 0};
            if (!empty.ptr) {
                return {nullptr, 0};
            }

            *empty.ptr = 0;

            return empty;
        }

        // 第一次遍历：计算有效半字节个数
        usize validNibbleCount = 0;
        for (const char* ptr = hexStr; *ptr != '\0'; ++ptr) {
            // 跳过空格
            if (isspace((uchar) *ptr)) {
                continue;
            }

            // 无效半字节
            const int nibble = hexCharToNibble(*ptr);
            if (nibble == -1) {
                return {nullptr, 0};
            }

            ++validNibbleCount;
        }

        // 奇数个半字节无法配对，十六进制文本必须成对出现
        if (validNibbleCount % 2 != 0) {
            return {nullptr, 0};
        }

        // 分配结果内存
        const usize resultLen = validNibbleCount / 2;
        const auto result = Span<u8>{(u8*) malloc(resultLen), resultLen};
        if (!result.ptr) {
            return {nullptr, 0};
        }

        u8* writePtr = result.ptr;

        // 准备半字节索引记录、当前字节记录
        u8 currentByte = 0;
        bool isUpperNibble = true;

        // 逐字符遍历，读取并组装高低半字符
        for (const char* ptr = hexStr; *ptr != '\0'; ++ptr) {
            // 跳过空格
            if (isspace((uchar) *ptr)) {
                continue;
            }

            // 读当前半字节
            const int nibble = hexCharToNibble(*ptr);

            // 无效半字节
            if (nibble == -1) {
                free(result.ptr);
                return {nullptr, 0};
            }

            if (isUpperNibble) {
                // 高半字符，读高半位，等待继续读低半位
                const u8 upper = nibble << 4;
                currentByte = upper;
                isUpperNibble = false;
            } else {
                // 低半字符，读低半位，拼接为单个字节写入临时游标，重置状态
                const u8 lower = nibble & 0x0F;
                currentByte |= lower;
                *writePtr++ = currentByte;
                isUpperNibble = true;
            }
        }

        return result;
    }
}
