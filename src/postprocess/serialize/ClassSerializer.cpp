#include "ClassSerializer.hpp"
#include "../../util/ByteUtils.hpp"

namespace JByteDom::postprocess::serialize {
    namespace {
        constexpr u32 JAVA_CLASS_MAGIC = 0xCAFEBABE;
    }

    namespace {
        auto serializeConstantPool(
            model::common::ElasticArray<u8>& bytesOut,
            const model::clazz::ClassMetadata& classMetadata
        ) -> void {
            util::writeU2(bytesOut, classMetadata.constantPoolCount);
            for (usize i = 1; i < classMetadata.constantPool.getSize(); ++i) {
                const auto& entry = classMetadata.constantPool[i];
                util::writeU1(bytesOut, entry.tag);

                switch (entry.tag) {
                    using namespace model::clazz::ConstantPoolEntryTag;
                    case Utf8: {
                        util::writeU2(bytesOut, entry.utf8Len);
                        for (usize j = 0; j < entry.utf8Len; ++j) {
                            bytesOut.push((u8) entry.utf8Str[j]);
                        }
                        break;
                    }
                    case Integer: {
                        util::writeU4(bytesOut, entry.intValue);
                        break;
                    }
                    case Float: {
                        util::writeU4(bytesOut, entry.intValue);
                        break;
                    }
                    case Long: {
                        util::writeU8(bytesOut, entry.longValue);
                        break;
                    }
                    case Double: {
                        util::writeU8(bytesOut, entry.longValue);
                        break;
                    }
                    case Class: {
                        util::writeU2(bytesOut, entry.nameIndex);
                        break;
                    }
                    case String: {
                        util::writeU2(bytesOut, entry.nameIndex);
                        break;
                    }
                    case Module: {
                        util::writeU2(bytesOut, entry.nameIndex);
                        break;
                    }
                    case Package: {
                        util::writeU2(bytesOut, entry.nameIndex);
                        break;
                    }
                    case Fieldref: {
                        util::writeU2(bytesOut, entry.classIndex);
                        util::writeU2(bytesOut, entry.nameAndTypeIndex);
                        break;
                    }
                    case Methodref: {
                        util::writeU2(bytesOut, entry.classIndex);
                        util::writeU2(bytesOut, entry.nameAndTypeIndex);
                        break;
                    }
                    case InterfaceMethodref: {
                        util::writeU2(bytesOut, entry.classIndex);
                        util::writeU2(bytesOut, entry.nameAndTypeIndex);
                        break;
                    }
                    case NameAndType: {
                        util::writeU2(bytesOut, entry.nameIndex);
                        util::writeU2(bytesOut, entry.descriptorIndex);
                        break;
                    }
                    case MethodHandle: {
                        util::writeU1(bytesOut, entry.referenceType);
                        util::writeU2(bytesOut, entry.referenceIndex);
                        break;
                    }
                    case MethodType: {
                        util::writeU2(bytesOut, entry.descriptorIndex);
                        break;
                    }
                    case Dynamic: {
                        util::writeU2(bytesOut, entry.bootstrapAttributeIndex);
                        util::writeU2(bytesOut, entry.nameAndTypeIndex);
                        break;
                    }
                    case InvokeDynamic: {
                        util::writeU2(bytesOut, entry.bootstrapAttributeIndex);
                        util::writeU2(bytesOut, entry.nameAndTypeIndex);
                        break;
                    }
                    default: break;
                }
            }
        }

        auto serializeSingleAttribute(
            model::common::ElasticArray<u8>& bytesOut,
            const model::clazz::AttributeInfo& attributeInfo
        ) -> void {
            util::writeU2(bytesOut, attributeInfo.attributeNameIndex);
            util::writeU4(bytesOut, attributeInfo.attributeLength);
            for (usize i = 0; i < attributeInfo.attributeBytes.getSize(); ++i) {
                bytesOut.push(attributeInfo.attributeBytes[i]);
            }
        }

        auto serializeAttributes(
            model::common::ElasticArray<u8>& bytesOut,
            const model::common::ElasticArray<model::clazz::AttributeInfo>& attributeInfos
        ) -> void {
            util::writeU2(bytesOut, (u16) attributeInfos.getSize());
            for (usize i = 0; i < attributeInfos.getSize(); ++i) {
                const auto& attributeInfo = attributeInfos[i];
                serializeSingleAttribute(bytesOut, attributeInfo);
            }
        }

        auto serializeFields(
            model::common::ElasticArray<u8>& bytesOut,
            const model::clazz::ClassMetadata& classMetadata
        ) -> void {
            util::writeU2(bytesOut, classMetadata.fieldsCount);
            for (usize i = 0; i < classMetadata.fields.getSize(); ++i) {
                const auto& fieldInfo = classMetadata.fields[i];
                util::writeU2(bytesOut, fieldInfo.accessFlags);
                util::writeU2(bytesOut, fieldInfo.nameIndex);
                util::writeU2(bytesOut, fieldInfo.descriptorIndex);
                serializeAttributes(bytesOut, fieldInfo.attributes);
            }
        }

        auto serializeMethods(
            model::common::ElasticArray<u8>& bytesOut,
            const model::clazz::ClassMetadata& classMetadata
        ) -> void {
            util::writeU2(bytesOut, classMetadata.methodsCount);
            for (usize i = 0; i < classMetadata.methods.getSize(); ++i) {
                const auto& methodInfo = classMetadata.methods[i];

                util::writeU2(bytesOut, methodInfo.accessFlags);
                util::writeU2(bytesOut, methodInfo.nameIndex);
                util::writeU2(bytesOut, methodInfo.descriptorIndex);

                u16 attributeCount = 0;
                if (methodInfo.hasCode) {
                    ++attributeCount;
                }
                attributeCount += (u16) methodInfo.attributes.getSize();
                util::writeU2(bytesOut, attributeCount);

                if (methodInfo.hasCode) {
                    auto codeAttributeInfo = model::clazz::AttributeInfo{};
                    codeAttributeInfo.attributeNameIndex = methodInfo.codeAttributeNameIndex;

                    auto codeBytes = model::common::ElasticArray<u8>();

                    util::writeU2(codeBytes, methodInfo.maxStack);
                    util::writeU2(codeBytes, methodInfo.maxLocals);

                    util::writeU4(codeBytes, (u32) methodInfo.bytecodes.getSize());
                    for (usize j = 0; j < methodInfo.bytecodes.getSize(); ++j) {
                        codeBytes.push(methodInfo.bytecodes[j]);
                    }

                    util::writeU2(codeBytes, (u16) (methodInfo.exceptionTableBytes.getSize() / 8));
                    for (usize j = 0; j < methodInfo.exceptionTableBytes.getSize(); ++j) {
                        codeBytes.push(methodInfo.exceptionTableBytes[j]);
                    }

                    serializeAttributes(codeBytes, methodInfo.codeAttributes);

                    codeAttributeInfo.attributeLength = (u32) codeBytes.getSize();
                    codeAttributeInfo.attributeBytes = std::move(codeBytes);

                    serializeSingleAttribute(bytesOut, codeAttributeInfo);
                }

                for (usize j = 0; j < methodInfo.attributes.getSize(); ++j) {
                    serializeSingleAttribute(bytesOut, methodInfo.attributes[j]);
                }
            }
        }
    }

    auto ClassSerializer::serializeClassMetadata(
        const model::clazz::ClassMetadata& classMetadata
    ) -> model::common::ElasticArray<u8> {
        auto bytes = model::common::ElasticArray<u8>();

        util::writeU4(bytes, JAVA_CLASS_MAGIC);
        util::writeU2(bytes, classMetadata.minorVersion);
        util::writeU2(bytes, classMetadata.majorVersion);

        serializeConstantPool(bytes, classMetadata);

        util::writeU2(bytes, classMetadata.accessFlags);
        util::writeU2(bytes, classMetadata.thisClass);
        util::writeU2(bytes, classMetadata.superClass);

        util::writeU2(bytes, classMetadata.interfacesCount);
        for (usize i = 0; i < classMetadata.interfaces.getSize(); ++i) {
            util::writeU2(bytes, classMetadata.interfaces[i]);
        }

        serializeFields(bytes, classMetadata);
        serializeMethods(bytes, classMetadata);
        serializeAttributes(bytes, classMetadata.attributes);

        return bytes;
    }
}
