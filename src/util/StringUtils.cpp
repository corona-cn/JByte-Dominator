#include "StringUtils.hpp"

#include <cstdarg>

#include "../CommonPrimitives.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace JByteDom::util {
    auto makeStr(
        const char* firstStr,
        ...
    ) -> char* {
        // 首参数为空指针，直接返回空指针
        if (!firstStr) {
            return nullptr;
        }

        // 准备记录总字符串长度以及有效参数数量
        usize totalStrLen = 0;
        int argCount = 0;

        va_list args;

        //第一次遍历，记录总字符串长度以及有效参数数量
        va_start(args, firstStr); {
            const char* currentChar = firstStr;
            while (currentChar) {
                totalStrLen += strlen(currentChar);
                argCount++;
                currentChar = va_arg(args, const char*);
            }
        } va_end(args);

        // 没有有效参数，返回空指针
        if (argCount == 0) {
            return nullptr;
        }

        // 计算分隔字符数量
        const usize separatorCount = argCount - 1;

        // 分配结果内存：有效字符串长度和 + 分隔字符数量 (1/byte) + 尾空字符 (1/byte)
        const auto result = (char*) malloc(totalStrLen + separatorCount + 1);

        // 内存分配失败，返回空指针
        if (!result) {
            return nullptr;
        }

        // 第二次遍历，正式组装结果
        va_start(args, firstStr); {
            // 初始化当前字符为首位参数字符
            const char* currentChar = firstStr;

            char* writePtr = result;

            // 逐参数遍历
            for (int i = 0; i < argCount; i++) {
                // 给非首参数的参数前补上分隔符
                if (i > 0) {
                    *writePtr++ = '/';
                }

                // 计算当前字符长度
                const usize currentCharLen = strlen(currentChar);

                // 拷贝当前字符内存到临时游标处
                memcpy(writePtr, currentChar, currentCharLen);

                // 向前移动写指针
                writePtr += currentCharLen;

                // 获取参数列表中的下一个字符串，同时向前移动隐式参数指针
                currentChar = va_arg(args, const char*);
            }

            // 补齐尾空字符，标志结束
            *writePtr = '\0';
        } va_end(args);

        return result;
    }

    auto makeEmptyStr() -> char* {
        const auto result = (char*) malloc(1);
        if (!result) {
            return nullptr;
        }

        result[0] = '\0';

        return result;
    }

    #ifdef _WIN32
    auto widenUtf8Dup(
        const char* utf8Str
    ) -> wchar* {
        const int wideStrLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
        if (wideStrLen <= 0) {
            return nullptr;
        }

        const auto wideStr = (wchar*) malloc(wideStrLen * sizeof(wchar));
        if (!wideStr) {
            return nullptr;
        }

        MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wideStr, wideStrLen);

        return wideStr;
    }
    #endif

    auto replaceAllCharsDup(
        const char* str,
        const char oldChar,
        const char newChar
    ) -> char* {
        if (!str) {
            return nullptr;
        }

        if (oldChar == newChar) {
            const auto result = (char*) malloc(strlen(str) + 1);
            if (!result) {
                return nullptr;
            }

            strcpy(result, str);

            return result;
        }

        const auto result = (char*) malloc(strlen(str) + 1);
        if (!result) {
            return nullptr;
        }

        // 逐字符遍历替换
        char* writePtr = result;
        for (const char* ptr = str; *ptr; ++ptr) {
            *writePtr++ = (*ptr == oldChar) ? newChar : *ptr;
        }

        // 补上尾空字符
        *writePtr = '\0';

        return result;
    }
}
