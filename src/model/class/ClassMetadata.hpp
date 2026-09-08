#pragma once
#include "../../CommonPrimitives.hpp"
#include "../common/ElasticArray.hpp"

namespace JByteDom::model::clazz {
    namespace ConstantPoolEntryTag {
        constexpr u8 Utf8 = 1;
        constexpr u8 Integer = 3;
        constexpr u8 Float = 4;
        constexpr u8 Long = 5;
        constexpr u8 Double = 6;
        constexpr u8 Class = 7;
        constexpr u8 String = 8;
        constexpr u8 Fieldref = 9;
        constexpr u8 Methodref = 10;
        constexpr u8 InterfaceMethodref = 11;
        constexpr u8 NameAndType = 12;
        constexpr u8 MethodHandle = 15;
        constexpr u8 MethodType = 16;
        constexpr u8 Dynamic = 17;
        constexpr u8 InvokeDynamic = 18;
        constexpr u8 Module = 19;
        constexpr u8 Package = 20;
    }

    namespace AccessFlag {
        namespace Class {
            constexpr u16 Public = 0x0001;
            constexpr u16 Final = 0x0010;
            constexpr u16 Super = 0x0020;
            constexpr u16 Interface = 0x0200;
            constexpr u16 Abstract = 0x0400;
            constexpr u16 Synthetic = 0x1000;
            constexpr u16 Annotation = 0x2000;
            constexpr u16 Enum = 0x4000;
            constexpr u16 Module = 0x8000;
        }
        namespace Field {
            constexpr u16 Public = 0x0001;
            constexpr u16 Private = 0x0002;
            constexpr u16 Protected = 0x0004;
            constexpr u16 Static = 0x0008;
            constexpr u16 Final = 0x0010;
            constexpr u16 Volatile = 0x0040;
            constexpr u16 Transient = 0x0080;
            constexpr u16 Synthetic = 0x1000;
            constexpr u16 Enum = 0x4000;
        }
        namespace Method {
            constexpr u16 Public = 0x0001;
            constexpr u16 Private = 0x0002;
            constexpr u16 Protected = 0x0004;
            constexpr u16 Static = 0x0008;
            constexpr u16 Final = 0x0010;
            constexpr u16 Synchronized = 0x0020;
            constexpr u16 Bridge = 0x0040;
            constexpr u16 Varargs = 0x0080;
            constexpr u16 Native = 0x0100;
            constexpr u16 Abstract = 0x0400;
            constexpr u16 Strictfp = 0x0800;
            constexpr u16 Synthetic = 0x1000;
        }
    }

    struct ConstantPoolEntry {
        u8 tag;

        u16 nameIndex;
        u16 classIndex;
        u16 nameAndTypeIndex;
        u16 descriptorIndex;
        u16 bootstrapAttributeIndex;

        u16 utf8Len;
        char* utf8Str;

        u32 intValue;
        u64 longValue;

        u8 referenceType;
        u16 referenceIndex;
    };

    struct AttributeInfo {
        u16 attributeNameIndex;

        const char* attributeName;
        u32 attributeLength;
        common::ElasticArray<u8> attributeBytes;
    };

    struct FieldInfo {
        u16 accessFlags;
        u16 nameIndex;
        u16 descriptorIndex;

        const char* name;
        const char* descriptor;

        common::ElasticArray<AttributeInfo> attributes;

        auto getAccessFlagsStr() const -> const char*;
        auto getDisplayFullName() const -> const char*;
    };

    struct MethodInfo {
        u16 accessFlags;
        u16 nameIndex;
        u16 descriptorIndex;

        const char* internalName;
        char* qualifiedName;
        const char* descriptor;

        common::ElasticArray<AttributeInfo> attributes;

        bool hasCode = false;
        u16 codeAttributeNameIndex = 0;
        u16 maxStack = 0;
        u16 maxLocals = 0;
        u32 codeOffset = 0;
        u32 codeLength = 0;
        common::ElasticArray<u8> bytecodes;

        common::ElasticArray<AttributeInfo> codeAttributes;

        common::ElasticArray<u8> exceptionTableBytes;

        auto getAccessFlagsStr() const -> const char*;
        auto getDisplayFullName() const -> const char*;
    };

    class ClassMetadata {
        public:
            ClassMetadata() = default;
            ~ClassMetadata();

            // 禁止拷贝构造和拷贝赋值
            ClassMetadata(const ClassMetadata&) = delete;
            ClassMetadata& operator = (const ClassMetadata&) = delete;

            ClassMetadata(ClassMetadata&& other) noexcept;
            ClassMetadata& operator = (ClassMetadata&& other) noexcept;

            u32 magic;
            u16 minorVersion;
            u16 majorVersion;

            u16 constantPoolCount;
            common::ElasticArray<ConstantPoolEntry> constantPool;

            u16 accessFlags;
            u16 thisClass;
            u16 superClass;
            u16 interfacesCount;
            common::ElasticArray<u16> interfaces;

            u16 fieldsCount;
            common::ElasticArray<FieldInfo> fields;

            u16 methodsCount;
            common::ElasticArray<MethodInfo> methods;

            u16 attributesCount;
            common::ElasticArray<AttributeInfo> attributes;

            const char* thisClassInternalName;
            char* thisClassQualifiedName;
            const char* superClassInternalName;
            char* superClassQualifiedName;
            common::ElasticArray<const char*> utf8Cache;

            auto getJavaVersionStr() const -> const char*;
            auto getAccessFlagsStr() const -> const char*;
            auto getDisplayFullName() const -> const char*;

            auto findFieldByName(const char* name) const -> const FieldInfo*;

            auto findMethodsByInternalName(const char* internalName) const -> common::ElasticArray<const MethodInfo*>;
            auto findMethodsByQualifiedName(const char* qualifiedName) const -> common::ElasticArray<const MethodInfo*>;

            auto addUtf8Entry(const char* str) -> u16;
            auto addClassEntry(const char* internalName) -> u16;
            auto addNameAndTypeEntry(const char* name, const char* descriptor) -> u16;
            auto addFieldrefEntry(const char* className, const char* fieldName, const char* descriptor) -> u16;
            auto addMethodrefEntry(const char* className, const char* methodName, const char* descriptor) -> u16;
            auto addStringEntry(const char* str) -> u16;
    };
}
