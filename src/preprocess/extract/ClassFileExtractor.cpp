#include "ClassFileExtractor.hpp"

namespace JByteDom::preprocess::extract {
    namespace {
        constexpr u32 JAVA_CLASS_MAGIC = 0xCAFEBABE;
    }

    auto ClassFileExtractor::extractAllClassFiles(
        model::jar::JarFile& jarFile,
        model::common::ElasticArray<model::jar::JarEntry>& jarEntries
    ) -> model::common::ElasticArray<model::clazz::ClassFile> {
        auto classFiles = model::common::ElasticArray<model::clazz::ClassFile>();
        if (jarEntries.isEmpty()) {
            return classFiles;
        }

        for (usize i = 0; i < jarEntries.getSize(); ++i) {
            auto& jarEntry = jarEntries[i];
            if (jarEntry.getEntryType() != model::jar::EntryType::File) {
                continue;
            }

            const auto ext = jarEntry.getExtension();
            if (ext && strcmp(ext, "class") != 0) {
                continue;
            }

            const u64 bytesLen = jarEntry.getBytesLen();
            if (bytesLen < 4) {
                continue;
            }

            u8* bytes = jarEntry.extractBytes(jarFile);
            if (!bytes) {
                continue;
            }

            const u32 magic = (u32) bytes[0] << 24 | (u32) bytes[1] << 16 | (u32) bytes[2] << 8 | (u32) bytes[3];
            if (magic == JAVA_CLASS_MAGIC) {
                auto classFile = model::clazz::ClassFile(
                    jarEntry.getPath(),
                    jarEntry.getBytesLen(),
                    bytes
                );

                classFiles.push(std::move(classFile));
            } else {
                free(bytes);
            }
        }

        return classFiles;
    }
}
