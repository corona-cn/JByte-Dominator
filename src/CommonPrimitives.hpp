#pragma once
#include <cstdint>
#include <cstddef>

namespace JByteDom {
    /* === 基本类型别名 === */
    /* 有符号整数 */
    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    /* 无符号整数 */
    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using uint = unsigned int;


    /* === 基本大小类型别名 === */
    /* 有符号大小 */
    using ssize = std::ptrdiff_t;

    /* 无符号大小 */
    using usize = size_t;


    /* === 基本指针类型别名 === */
    /* 指针指向的地址 */
    using addr = uintptr_t;


    /* === 基本字符类型别名 === */
    /* 有符号字符 */
    using schar = signed char;

    /* 无符合字符 */
    using uchar = unsigned char;


    /* === 平台特定字符类型别名 === */
    /* 宽字符 */
    #ifdef _WIN32
    using wchar = wchar_t;
    #endif
}