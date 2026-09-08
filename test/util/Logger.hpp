#pragma once
#include <algorithm>
#include <iostream>

namespace JByteDom::test::util {
    inline std::string formatLineNumber(const int line) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%05d", line);
        return std::string(buf);
    }

    template <typename T, typename... Args>
    auto log(const T& first, const Args&... rest) -> void {
        std::cout << first;
        (std::cout << ... << rest);
        std::cout << std::endl;
    }

    inline int& indentLevel() {
        static int level = 0;
        return level;
    }
    inline void indentPush() {
        ++indentLevel();
    }
    inline void indentPop() {
        if (indentLevel() > 0) {
            --indentLevel();
        }
    }
    inline std::string getIndent() {
        std::string result;

        const int level = indentLevel();
        result.reserve(level * 3);
        for (int i = 0; i < level; ++i) {
            result += "  |";
        }

        return result;
    }

    inline const char* getRelativeFile(const char* file) {
        static const char* cached = nullptr;
        if (cached == nullptr) {
            std::string fileStr(file);
            std::string rootStr(PROJECT_SOURCE_DIR);

            for (char& c : fileStr) {
                if (c == '\\') {
                    c = '/';
                }
            }

            for (char& c : rootStr) {
                if (c == '\\') {
                    c = '/';
                }
            }

            std::string fileLower = fileStr;
            std::string rootLower = rootStr;
            std::transform(fileLower.begin(), fileLower.end(), fileLower.begin(), tolower);
            std::transform(rootLower.begin(), rootLower.end(), rootLower.begin(), tolower);

            const size_t pos = fileLower.find(rootLower);
            if (pos != std::string::npos) {
                cached = file + rootStr.length();
                while (*cached == '/' || *cached == '\\') {
                    ++cached;
                }
            } else {
                cached = file;
            }
        }

        return cached;
    }
}

#define CONCAT_IMPL(a, b) \
    a##b

#define CONCAT(a, b) \
    CONCAT_IMPL(a, b)

#define LOGGER_INDENT_BLOCK(enabled) \
    for (int CONCAT(_indent_, __LINE__) = \
         (::JByteDom::test::util::indentPush(), \
          (enabled) ? 0 : (LOG("── 折叠块 ──"), 0)); \
         CONCAT(_indent_, __LINE__) < 1; \
         (::JByteDom::test::util::indentPop(), ++CONCAT(_indent_, __LINE__))) \
        if (enabled)

#define LOG(...) \
    JByteDom::test::util::log( \
        "[", JByteDom::test::util::getRelativeFile(__FILE__), ":", \
        JByteDom::test::util::formatLineNumber(__LINE__), \
        "] ", \
        JByteDom::test::util::getIndent(), \
        __VA_ARGS__ \
    )

#define PURE_LOG(...) \
    JByteDom::test::util::log( \
        "[", JByteDom::test::util::getRelativeFile(__FILE__), ":", \
        JByteDom::test::util::formatLineNumber(__LINE__), \
        "] ", \
        __VA_ARGS__ \
    )