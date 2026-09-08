#pragma once
#include <cstdlib>

/* === 指针管理宏 === */
/* 通用指针 */
#define PTR_FREE_AND_NULL(ptr) \
    do { \
        free(ptr); \
        ptr = nullptr; \
    } while (0)

/* 文件指针 */
#define FILE_CLOSE_AND_NULL(file) \
    do { \
        fclose(file); \
        file = nullptr; \
    } while (0)