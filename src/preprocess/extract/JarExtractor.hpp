#pragma once
#include "../../model/common/ElasticArray.hpp"
#include "../../model/jar/JarEntry.hpp"
#include "../../model/jar/JarFile.hpp"

namespace JByteDom::preprocess::extract {
    class JarExtractor {
        public:
            explicit JarExtractor(const model::jar::JarFile& jarFile);
            ~JarExtractor() = default;

            JarExtractor(const JarExtractor&) = delete;
            JarExtractor& operator=(const JarExtractor&) = delete;

            JarExtractor(JarExtractor&&) = delete;
            JarExtractor& operator=(JarExtractor&&) = delete;

            auto extractAllEntries() -> model::common::ElasticArray<model::jar::JarEntry>;

            auto getError() const -> const char* { return this->error; }


        private:
            const model::jar::JarFile& jarFile;
            const char* error = nullptr;
    };
}
