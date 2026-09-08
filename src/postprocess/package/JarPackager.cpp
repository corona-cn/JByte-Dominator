#include "JarPackager.hpp"
#include <zip.h>
#include <cstring>
#include <string>
#include <unordered_map>

#include "../../util/ByteUtils.hpp"
#include "../serialize/ClassSerializer.hpp"

namespace JByteDom::postprocess::package {
    auto JarPackager::packageJar(
        const char* inputJarPath,
        const char* outputJarPath,
        const model::common::ElasticArray<model::clazz::ClassMetadata>& modifiedClassMetadatas
    ) -> bool {
        this->error = nullptr;

        int errorCode = 0;
        zip_t* inputZip = zip_open(inputJarPath, ZIP_RDONLY, &errorCode);
        if (!inputZip) {
            this->error = "无法打开输入 JAR 文件";
            return false;
        }

        zip_t* outputZip = zip_open(outputJarPath, ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!outputZip) {
            zip_close(inputZip);
            this->error = "无法创建输出 JAR 文件";
            return false;
        }

        std::unordered_map<std::string, model::common::ElasticArray<u8>> modifiedMap;
        const usize classCount = modifiedClassMetadatas.getSize();
        for (usize i = 0; i < classCount; ++i) {
            const auto& md = modifiedClassMetadatas[i];
            if (md.thisClassInternalName) {
                std::string className = md.thisClassInternalName;
                modifiedMap[className] = serialize::ClassSerializer::serializeClassMetadata(md);
            }
        }

        const zip_int64_t totalEntries = zip_get_num_entries(inputZip, 0);
        for (zip_int64_t index = 0; index < totalEntries; ++index) {
            const char* entryName = zip_get_name(inputZip, index, 0);
            if (!entryName) {
                continue;
            }

            bool isModified = false;
            const usize nameLen = strlen(entryName);
            if (nameLen > 6 && strcmp(entryName + nameLen - 6, ".class") == 0) {
                std::string className(entryName, nameLen - 6);
                if (modifiedMap.find(className) != modifiedMap.end()) {
                    isModified = true;
                }
            }

            if (isModified) {
                continue;
            }

            struct zip_stat statBuffer;
            zip_stat_init(&statBuffer);
            if (zip_stat_index(inputZip, index, 0, &statBuffer) != 0) {
                continue;
            }

            zip_file_t* file = zip_fopen_index(inputZip, index, 0);
            if (!file) {
                continue;
            }

            const auto data = (u8*) malloc(statBuffer.size);
            if (!data) {
                zip_fclose(file);
                continue;
            }

            const zip_int64_t bytesRead = zip_fread(file, data, statBuffer.size);
            zip_fclose(file);

            if (bytesRead != (zip_int64_t) statBuffer.size) {
                free(data);
                continue;
            }

            zip_source_t* source = zip_source_buffer(outputZip, data, statBuffer.size, 1);
            if (!source) {
                free(data);
                continue;
            }

            const zip_int64_t existing = zip_name_locate(outputZip, entryName, 0);
            if (existing >= 0) {
                zip_delete(outputZip, existing);
            }

            if (zip_file_add(outputZip, entryName, source, 0) < 0) {
                zip_source_free(source);
            }
        }

        for (auto& pair : modifiedMap) {
            const std::string& className = pair.first;
            model::common::ElasticArray<u8>& bytes = pair.second;

            if (bytes.getSize() == 0) {
                continue;
            }

            std::string fullEntryName = className + ".class";

            const auto classData = (u8*) malloc(bytes.getSize());
            if (!classData) {
                continue;
            }
            memcpy(classData, bytes.peekFirst(), bytes.getSize());

            zip_source_t* source = zip_source_buffer(outputZip, classData, bytes.getSize(), 1);
            if (!source) {
                free(classData);
                continue;
            }

            const zip_int64_t existing = zip_name_locate(outputZip, fullEntryName.c_str(), 0);
            if (existing >= 0) {
                zip_delete(outputZip, existing);
            }

            if (zip_file_add(outputZip, fullEntryName.c_str(), source, 0) < 0) {
                zip_source_free(source);
            }
        }

        zip_close(inputZip);
        zip_close(outputZip);

        this->error = nullptr;
        return true;
    }
}