#pragma once
#include "../../model/class/ClassMetadata.hpp"
#include "../../model/common/ElasticArray.hpp"

namespace JByteDom::postprocess::package {
    class JarPackager {
        public:
            auto packageJar(
                const char* inputJarPath,
                const char* outputJarPath,
                const model::common::ElasticArray<model::clazz::ClassMetadata>& modifiedClassMetadatas
            ) -> bool;

            auto getError() const -> const char* { return this->error; }


        private:
            const char* error = nullptr;
    };
}
