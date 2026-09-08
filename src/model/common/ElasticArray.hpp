#pragma once
#include <algorithm>

#include "../../CommonMacro.hpp"
#include "../../CommonPrimitives.hpp"

namespace JByteDom::model::common {
    template<typename Type> class ElasticArray {
        public:
            ElasticArray() = default;
            ~ElasticArray() {
                for (usize i = 0; i < this->dataSize; ++i) {
                    // 调用每一个元素的析构函数
                    (this->dataPtr + i)->~Type();
                }

                // 释放数据流指针
                free(this->dataPtr);
            }

            // 禁止拷贝构造和拷贝赋值
            ElasticArray(const ElasticArray&) = delete;
            ElasticArray& operator = (const ElasticArray&) = delete;

            ElasticArray(ElasticArray&& other) noexcept:
                dataPtr(other.dataPtr),
                dataSize(other.dataSize),
                capacity(other.capacity)
            {
                other.dataPtr = nullptr;
                other.dataSize = 0;
                other.capacity = 0;
            }
            ElasticArray& operator = (ElasticArray&& other) noexcept {
                if (this != &other) {
                    for (usize i = 0; i < this->dataSize; ++i) {
                        (this->dataPtr + i)->~Type();
                    }
                    free(this->dataPtr);

                    this->dataPtr = other.dataPtr;
                    this->dataSize = other.dataSize;
                    this->capacity = other.capacity;

                    other.dataPtr = nullptr;
                    other.dataSize = 0;
                    other.capacity = 0;
                }
                return *this;
            }

            auto push(const Type& value) -> bool {
                if (!ensureCapacity()) {
                    return false;
                }

                // 在元素尾部调用 Type 构造函数，现场构造并占据目标内存
                new (this->dataPtr + this->dataSize) Type(value);

                ++this->dataSize;
                return true;
            }
            auto push(Type&& value) -> bool {
                if (!ensureCapacity()) {
                    return false;
                }

                new (this->dataPtr + this->dataSize) Type(std::move(value));

                ++this->dataSize;
                return true;
            }

            auto pop() -> void {
                if (this->dataSize > 0) {
                    --this->dataSize;
                    (this->dataPtr + this->dataSize)->~Type();
                }
            }

            auto insertAt(usize index, const Type& value) -> bool {
                if (index > this->dataSize) {
                    return false;
                }

                if (!ensureCapacity()) {
                    return false;
                }

                for (usize i = this->dataSize; i > index; --i) {
                    new (this->dataPtr + i) Type(std::move(*(this->dataPtr + i - 1)));
                }

                (this->dataPtr + index)->~Type();

                new (this->dataPtr + index) Type(value);

                ++this->dataSize;
                return true;
            }

            auto removeAt(usize index) -> void {
                if (index >= this->dataSize) {
                    return;
                }

                (this->dataPtr + index)->~Type();

                for (usize i = index + 1; i < this->dataSize; ++i) {
                    new (this->dataPtr + i - 1) Type(std::move(*(this->dataPtr + i)));
                    (this->dataPtr + i)->~Type();
                }

                --this->dataSize;
            }

            auto clear() -> void {
                for (usize i = 0; i < this->dataSize; ++i) {
                    (this->dataPtr + i)->~Type();
                }
                this->dataSize = 0;
            }

            auto reserve(usize newCapacity) -> bool {
                if (newCapacity <= this->capacity) {
                    return true;
                }

                Type* newDataPtr = (Type*) malloc(newCapacity * sizeof(Type));
                if (!newDataPtr) {
                    return false;
                }

                for (usize i = 0; i < this->dataSize; ++i) {
                    new (newDataPtr + i) Type(std::move(*(this->dataPtr + i)));
                    (this->dataPtr + i)->~Type();
                }

                free(this->dataPtr);
                this->dataPtr = newDataPtr;
                this->capacity = newCapacity;
                return true;
            }

            auto shrinkToFit() -> void {
                if (this->dataSize == 0) {
                    PTR_FREE_AND_NULL(this->dataPtr);
                    this->capacity = 0;
                    return;
                }
                reserve(this->dataSize);
            }

            auto begin() const -> Type* { return this->dataPtr; }
            auto peekFirst() const -> const Type* { return this->dataPtr; }
            auto end() const -> Type* { return this->dataSize > 0 ? this->dataPtr + this->dataSize - 1 : nullptr; }
            auto peekLast() const -> const Type* { return this->dataSize > 0 ? this->dataPtr + this->dataSize - 1 : nullptr; }

            auto getFirst() -> Type* { return this->dataPtr; }
            auto getLast() -> Type* { return this->dataSize > 0 ? this->dataPtr + this->dataSize - 1 : nullptr; }

            auto operator[](usize index) -> Type& { return *(this->dataPtr + index); }
            auto operator[](usize index) const -> const Type& { return *(this->dataPtr + index); }

            auto getSize() const -> usize { return this->dataSize; }
            auto getCapacity() const -> usize { return this->capacity; }
            auto isEmpty() const -> bool { return this->dataSize == 0; }


        private:
            Type* dataPtr = nullptr;
            usize dataSize = 0;
            usize capacity = 0;

            auto ensureCapacity() -> bool {
                if (this->dataSize < this->capacity) {
                    return true;
                }
                const usize newCapacity = this->capacity == 0 ? 4 : this->capacity * 2;
                return reserve(newCapacity);
            }
    };
}