#pragma once
#include "../../model/class/ClassMetadata.hpp"

namespace JByteDom::postprocess::serialize {
    class ClassSerializer {
        public:
            static auto serializeClassMetadata(
                const model::clazz::ClassMetadata& classMetadata
            ) -> model::common::ElasticArray<u8>;
    };
}
