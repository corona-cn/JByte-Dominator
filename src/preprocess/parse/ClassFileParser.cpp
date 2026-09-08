#include "ClassFileParser.hpp"
#include "../../util/ByteUtils.hpp"
#include "../../util/StringUtils.hpp"

namespace JByteDom::preprocess::parse {
    namespace {
        constexpr u32 JAVA_CLASS_MAGIC = 0xCAFEBABE;
    }

    namespace {
        auto parseConstantPoolEntry(
            model::clazz::ConstantPoolEntry& constantPoolEntry,
            const u8*& bytes,
            bool& isDoubleSlot
        ) -> bool {
            isDoubleSlot = false;
            constantPoolEntry.tag = util::readU1(bytes);

            switch (constantPoolEntry.tag) {
                using namespace model::clazz::ConstantPoolEntryTag;
                case Utf8: {
                    constantPoolEntry.utf8Len = util::readU2(bytes);
                    constantPoolEntry.utf8Str = (char*) malloc(constantPoolEntry.utf8Len + 1);
                    if (!constantPoolEntry.utf8Str) {
                        return false;
                    }

                    memcpy(constantPoolEntry.utf8Str, bytes, constantPoolEntry.utf8Len);
                    constantPoolEntry.utf8Str[constantPoolEntry.utf8Len] = '\0';
                    bytes += constantPoolEntry.utf8Len;

                    break;
                }
                case Integer: {
                    constantPoolEntry.intValue = util::readU4(bytes);
                    break;
                }
                case Float: {
                    constantPoolEntry.intValue = util::readU4(bytes);
                    break;
                }
                case Long: {
                    constantPoolEntry.longValue = util::readU8(bytes);
                    isDoubleSlot = true;
                    break;
                }
                case Double: {
                    constantPoolEntry.longValue = util::readU8(bytes);
                    isDoubleSlot = true;
                    break;
                }
                case Class: {
                    constantPoolEntry.nameIndex = util::readU2(bytes);
                    break;
                }
                case String: {
                    constantPoolEntry.nameIndex = util::readU2(bytes);
                    break;
                }
                case Module: {
                    constantPoolEntry.nameIndex = util::readU2(bytes);
                    break;
                }
                case Package: {
                    constantPoolEntry.nameIndex = util::readU2(bytes);
                    break;
                }
                case Fieldref: {
                    constantPoolEntry.classIndex = util::readU2(bytes);
                    constantPoolEntry.nameAndTypeIndex = util::readU2(bytes);
                    break;
                }
                case Methodref: {
                    constantPoolEntry.classIndex = util::readU2(bytes);
                    constantPoolEntry.nameAndTypeIndex = util::readU2(bytes);
                    break;
                }
                case InterfaceMethodref: {
                    constantPoolEntry.classIndex = util::readU2(bytes);
                    constantPoolEntry.nameAndTypeIndex = util::readU2(bytes);
                    break;
                }
                case NameAndType: {
                    constantPoolEntry.nameIndex = util::readU2(bytes);
                    constantPoolEntry.descriptorIndex = util::readU2(bytes);
                    break;
                }
                case MethodHandle: {
                    constantPoolEntry.referenceType = util::readU1(bytes);
                    constantPoolEntry.referenceIndex = util::readU2(bytes);
                    break;
                }
                case MethodType: {
                    constantPoolEntry.descriptorIndex = util::readU2(bytes);
                    break;
                }
                case Dynamic: {
                    constantPoolEntry.bootstrapAttributeIndex = util::readU2(bytes);
                    constantPoolEntry.nameAndTypeIndex = util::readU2(bytes);
                    break;
                }
                case InvokeDynamic: {
                    constantPoolEntry.bootstrapAttributeIndex = util::readU2(bytes);
                    constantPoolEntry.nameAndTypeIndex = util::readU2(bytes);
                    break;
                }
                default: return false;
            }

            return true;
        }

        auto parseConstantPool(
            model::clazz::ClassMetadata& classMetadata,
            const u8*& bytes
        ) -> bool {
            classMetadata.constantPoolCount = util::readU2(bytes);
            const u16 constantPoolCount = classMetadata.constantPoolCount;

            classMetadata.constantPool.reserve(constantPoolCount);
            classMetadata.constantPool.push(model::clazz::ConstantPoolEntry{});
            classMetadata.utf8Cache.reserve(constantPoolCount);
            classMetadata.utf8Cache.push(nullptr);

            for (u16 i = 1; i < constantPoolCount; ++i) {
                bool isDoubleSlot = false;
                auto entry = model::clazz::ConstantPoolEntry{};

                if (!parseConstantPoolEntry(entry, bytes, isDoubleSlot)) {
                    if (entry.tag == model::clazz::ConstantPoolEntryTag::Utf8 && entry.utf8Str) {
                        free(entry.utf8Str);
                    }

                    return false;
                }

                classMetadata.constantPool.push(std::move(entry));

                if (entry.tag == model::clazz::ConstantPoolEntryTag::Utf8) {
                    classMetadata.utf8Cache.push(entry.utf8Str);
                } else {
                    classMetadata.utf8Cache.push(nullptr);
                }

                if (isDoubleSlot) {
                    auto dummy = model::clazz::ConstantPoolEntry{};
                    classMetadata.constantPool.push(dummy);
                    classMetadata.utf8Cache.push(nullptr);
                    i++;
                }
            }

            return true;
        }

        auto parseAttributes(
            const model::clazz::ClassMetadata& classMetadata,
            const u8*& bytes,
            model::common::ElasticArray<model::clazz::AttributeInfo>& attributeInfos
        ) -> void;

        auto parseFields(
            model::clazz::ClassMetadata& classMetadata,
            const u8*& bytes
        ) -> void {
            classMetadata.fieldsCount = util::readU2(bytes);
            classMetadata.fields.reserve(classMetadata.fieldsCount);

            for (u16 i = 0; i < classMetadata.fieldsCount; ++i) {
                auto fieldInfo = model::clazz::FieldInfo{};

                fieldInfo.accessFlags = util::readU2(bytes);

                fieldInfo.nameIndex = util::readU2(bytes);
                fieldInfo.descriptorIndex = util::readU2(bytes);

                if (fieldInfo.nameIndex < classMetadata.utf8Cache.getSize()) {
                    fieldInfo.name = classMetadata.utf8Cache[fieldInfo.nameIndex];
                }

                if (fieldInfo.descriptorIndex < classMetadata.utf8Cache.getSize()) {
                    fieldInfo.descriptor = classMetadata.utf8Cache[fieldInfo.descriptorIndex];
                }

                parseAttributes(classMetadata, bytes, fieldInfo.attributes);

                classMetadata.fields.push(std::move(fieldInfo));
            }
        }

        auto parseMethods(
            model::clazz::ClassMetadata& classMetadata,
            const u8*& bytes,
            const u8* bytesStart
        ) -> void {
            classMetadata.methodsCount = util::readU2(bytes);
            classMetadata.methods.reserve(classMetadata.methodsCount);

            for (u16 i = 0; i < classMetadata.methodsCount; ++i) {
                auto methodInfo = model::clazz::MethodInfo{};

                methodInfo.accessFlags = util::readU2(bytes);

                methodInfo.nameIndex = util::readU2(bytes);
                methodInfo.descriptorIndex = util::readU2(bytes);

                if (methodInfo.nameIndex < classMetadata.utf8Cache.getSize()) {
                    methodInfo.internalName = classMetadata.utf8Cache[methodInfo.nameIndex];
                    methodInfo.qualifiedName = util::replaceAllCharsDup(methodInfo.internalName, '/', '.');
                }

                if (methodInfo.descriptorIndex < classMetadata.utf8Cache.getSize()) {
                    methodInfo.descriptor = classMetadata.utf8Cache[methodInfo.descriptorIndex];
                }

                const u16 attributeCount = util::readU2(bytes);
                for (u16 j = 0; j < attributeCount; ++j) {
                    const u16 attributeNameIndex = util::readU2(bytes);
                    const u32 attributeLen = util::readU4(bytes);

                    bool isCode = false;
                    if (attributeNameIndex < classMetadata.utf8Cache.getSize()) {
                        const char* attributeName = classMetadata.utf8Cache[attributeNameIndex];
                        if (attributeName && strcmp(attributeName, "Code") == 0) {
                            isCode = true;
                        }
                    }

                    if (isCode) {
                        methodInfo.hasCode = true;
                        methodInfo.codeAttributeNameIndex = attributeNameIndex;

                        const u8* codeStart = bytes;

                        methodInfo.maxStack = util::readU2(bytes);
                        methodInfo.maxLocals = util::readU2(bytes);

                        const u32 codeLen = util::readU4(bytes);
                        methodInfo.codeLength = codeLen;

                        methodInfo.codeOffset = (u32) (codeStart - bytesStart);

                        methodInfo.bytecodes.reserve(codeLen);
                        for (u32 k = 0; k < codeLen; ++k) {
                            methodInfo.bytecodes.push(util::readU1(bytes));
                        }

                        const u16 exceptionTableLen = util::readU2(bytes);
                        methodInfo.exceptionTableBytes.reserve(exceptionTableLen * 8);
                        for (u16 e = 0; e < exceptionTableLen; ++e) {
                            for (u8 b = 0; b < 8; ++b) {
                                methodInfo.exceptionTableBytes.push(util::readU1(bytes));
                            }
                        }

                        const u16 codeAttributeCount = util::readU2(bytes);
                        for (u16 k = 0; k < codeAttributeCount; ++k) {
                            auto subAttributeInfo = model::clazz::AttributeInfo{};
                            subAttributeInfo.attributeNameIndex = util::readU2(bytes);
                            subAttributeInfo.attributeLength = util::readU4(bytes);

                            if (subAttributeInfo.attributeNameIndex < classMetadata.utf8Cache.getSize()) {
                                subAttributeInfo.attributeName = classMetadata.utf8Cache[subAttributeInfo.attributeNameIndex];
                            }

                            subAttributeInfo.attributeBytes.reserve(subAttributeInfo.attributeLength);
                            for (u32 l = 0; l < subAttributeInfo.attributeLength; ++l) {
                                subAttributeInfo.attributeBytes.push(util::readU1(bytes));
                            }

                            methodInfo.codeAttributes.push(std::move(subAttributeInfo));
                        }
                    } else {
                        auto attributeInfo = model::clazz::AttributeInfo{};

                        attributeInfo.attributeNameIndex = attributeNameIndex;
                        if (attributeNameIndex < classMetadata.utf8Cache.getSize()) {
                            attributeInfo.attributeName = classMetadata.utf8Cache[attributeNameIndex];
                        }

                        attributeInfo.attributeLength = attributeLen;
                        attributeInfo.attributeBytes.reserve(attributeLen);

                        for (u32 k = 0; k < attributeLen; ++k) {
                            attributeInfo.attributeBytes.push(util::readU1(bytes));
                        }

                        methodInfo.attributes.push(std::move(attributeInfo));
                    }
                }

                classMetadata.methods.push(std::move(methodInfo));
            }
        }

        auto parseAttributes(
            const model::clazz::ClassMetadata& classMetadata,
            const u8*& bytes,
            model::common::ElasticArray<model::clazz::AttributeInfo>& attributeInfos
        ) -> void {
            const u16 attributeCount = util::readU2(bytes);
            attributeInfos.reserve(attributeCount);

            for (u16 i = 0; i < attributeCount; ++i) {
                auto attributeInfo = model::clazz::AttributeInfo{};

                attributeInfo.attributeNameIndex = util::readU2(bytes);
                attributeInfo.attributeLength = util::readU4(bytes);

                if (attributeInfo.attributeNameIndex < classMetadata.utf8Cache.getSize()) {
                    attributeInfo.attributeName = classMetadata.utf8Cache[attributeInfo.attributeNameIndex];
                }

                attributeInfo.attributeBytes.reserve(attributeInfo.attributeLength);
                for (u32 j = 0; j < attributeInfo.attributeLength; ++j) {
                    attributeInfo.attributeBytes.push(util::readU1(bytes));
                }

                attributeInfos.push(std::move(attributeInfo));
            }
        }
    }

    auto ClassFileParser::parseClassFile(
        const model::clazz::ClassFile& classFile
    ) -> model::clazz::ClassMetadata {
        // 构造类元数据结构体
        auto classMetadata = model::clazz::ClassMetadata{};

        // 获取类二进制数据
        const u8* bytesStart = classFile.getBytes();
        const u8* bytes = classFile.getBytes();

        // 读取类魔法头
        classMetadata.magic = util::readU4(bytes);
        if (classMetadata.magic != JAVA_CLASS_MAGIC) {
            return classMetadata;
        }

        // 读取类版本号
        classMetadata.minorVersion = util::readU2(bytes);
        classMetadata.majorVersion = util::readU2(bytes);

        // 解析类常量池
        if (!parseConstantPool(classMetadata, bytes)) {
            return classMetadata;
        }

        // 获取类信息
        classMetadata.accessFlags = util::readU2(bytes);
        classMetadata.thisClass = util::readU2(bytes);
        classMetadata.superClass = util::readU2(bytes);

        // 读取类接口表
        classMetadata.interfacesCount = util::readU2(bytes);
        classMetadata.interfaces.reserve(classMetadata.interfacesCount);
        for (u16 i = 0; i < classMetadata.interfacesCount; ++i) {
            classMetadata.interfaces.push(util::readU2(bytes));
        }

        // 解析类字段
        parseFields(classMetadata, bytes);

        // 解析类方法
        parseMethods(classMetadata, bytes, bytesStart);

        // 解析类属性
        parseAttributes(classMetadata, bytes, classMetadata.attributes);

        // 读取类名
        if (classMetadata.thisClass < classMetadata.utf8Cache.getSize()) {
            if (classMetadata.thisClass < classMetadata.constantPool.getSize()) {
                const auto& classEntry = classMetadata.constantPool[classMetadata.thisClass];
                if (classEntry.tag == model::clazz::ConstantPoolEntryTag::Class) {
                    const u16 nameIndex = classEntry.nameIndex;
                    if (nameIndex < classMetadata.utf8Cache.getSize()) {
                        classMetadata.thisClassInternalName = classMetadata.utf8Cache[nameIndex];
                        classMetadata.thisClassQualifiedName = util::replaceAllCharsDup(classMetadata.thisClassInternalName, '/', '.');
                    }
                }
            }
        }

        // 读取父类名
        if (classMetadata.superClass != 0 && classMetadata.superClass < classMetadata.utf8Cache.getSize()) {
            if (classMetadata.superClass < classMetadata.constantPool.getSize()) {
                const auto& classEntry = classMetadata.constantPool[classMetadata.superClass];
                if (classEntry.tag == model::clazz::ConstantPoolEntryTag::Class) {
                    const u16 nameIndex = classEntry.nameIndex;
                    if (nameIndex < classMetadata.utf8Cache.getSize()) {
                        classMetadata.superClassInternalName = classMetadata.utf8Cache[nameIndex];
                        classMetadata.superClassQualifiedName = util::replaceAllCharsDup(classMetadata.superClassInternalName, '/', '.');
                    }
                }
            }
        }

        return classMetadata;
    }

    auto ClassFileParser::parseClassFiles(
        const model::common::ElasticArray<model::clazz::ClassFile>& classFiles
    ) -> model::common::ElasticArray<model::clazz::ClassMetadata> {
        auto classMetadatas = model::common::ElasticArray<model::clazz::ClassMetadata>();
        const usize count = classFiles.getSize();

        classMetadatas.reserve(count);

        for (usize i = 0; i < count; ++i) {
            const auto& classFile = classFiles[i];
            auto classMetadata = parseClassFile(classFile);
            classMetadatas.push(std::move(classMetadata));
        }

        return classMetadatas;
    }
}
