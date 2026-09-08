#include "JarExtractor.hpp"

#include <zip.h>

namespace JByteDom::preprocess::extract {
    JarExtractor::JarExtractor(const model::jar::JarFile& jarFile):
        jarFile(jarFile)
    {}

    auto JarExtractor::extractAllEntries() -> model::common::ElasticArray<model::jar::JarEntry> {
        // 清空字段
        this->error = nullptr;

        // 构造 JarEntry 弹性数组
        auto entries = model::common::ElasticArray<model::jar::JarEntry>();

        // 构造 ZIP 错误结构体，并尝试创建 ZIP 数据源
        auto zipError = zip_error_t{};
        zip_error_init(&zipError);
        zip_source_t* source = zip_source_buffer_create(this->jarFile.getBytes(), this->jarFile.getLen(), 0, &zipError);
        if (!source) {
            this->error = "创建 ZIP 数据源失败";
            return entries;
        }

        // 尝试打开 ZIP 归档
        zip_t* zip = zip_open_from_source(source, ZIP_RDONLY, &zipError);
        if (!zip) {
            this->error = "打开 ZIP 归档失败";
            zip_source_free(source);
            return entries;
        }

        // 开始遍历 ZIP 包含的所有条目
        const zip_int64_t numEntries = zip_get_num_entries(zip, ZIP_FL_UNCHANGED);
        for (zip_int64_t i = 0; i < numEntries; ++i) {
            // 记录条目名字
            const char* name = zip_get_name(zip, i, ZIP_FL_ENC_GUESS);
            if (!name) {
                continue;
            }

            // 记录条目元数据
            struct zip_stat stat{};
            zip_stat_init(&stat);
            if (zip_stat_index(zip, i, 0, &stat) != 0) {
                continue;
            }

            // 构造 JarEntry
            auto jarEntry = model::jar::JarEntry(
                name,
                stat.size,
                i,
                stat.comp_size,
                stat.size,
                (model::jar::CompressionMethod) stat.comp_method
            );

            // 将 JarEntry 移动存入弹性数组
            entries.push(std::move(jarEntry));
        }

        // 关闭 ZIP
        zip_close(zip);

        return entries;
    }
}
