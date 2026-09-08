#pragma once
#include "CommonPrimitives.hpp"

namespace JByteDom {
    template<typename Type> struct Span {
        Type* ptr;
        usize len;
    };
}