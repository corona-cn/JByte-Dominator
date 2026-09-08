#pragma once
#include "../../model/class/ClassFile.hpp"
#include "../../model/class/ClassMetadata.hpp"

namespace JByteDom::preprocess::parse {
    class ClassFileParser {
        public:
            static auto parseClassFile(
                const model::clazz::ClassFile& classFile
            ) -> model::clazz::ClassMetadata;

            static auto parseClassFiles(
                const model::common::ElasticArray<model::clazz::ClassFile>& classFiles
            ) -> model::common::ElasticArray<model::clazz::ClassMetadata>;
    };
}
