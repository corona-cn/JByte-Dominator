#pragma once
#include "../../model/class/ClassFile.hpp"
#include "../../model/common/ElasticArray.hpp"
#include "../../model/jar/JarEntry.hpp"

namespace JByteDom::preprocess::extract {
    class ClassFileExtractor {
        public:
            static auto extractAllClassFiles(
                model::jar::JarFile& jarFile,
                model::common::ElasticArray<model::jar::JarEntry>& jarEntries
            ) -> model::common::ElasticArray<model::clazz::ClassFile>;
    };
}
