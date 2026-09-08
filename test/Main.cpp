#include <windows.h>

#include "../src/CommonStruct.hpp"
#include "util/Logger.hpp"
#include "../src/model/jar/JarFile.hpp"
#include "../src/postprocess/package/JarPackager.hpp"
#include "../src/preprocess/extract/ClassFileExtractor.hpp"
#include "../src/preprocess/extract/JarExtractor.hpp"
#include "../src/preprocess/parse/BytecodesParser.hpp"
#include "../src/preprocess/parse/ClassFileParser.hpp"
#include "../src/postprocess/serialize/ClassSerializer.hpp"
#include "../src/runtime/modify/InstructionsModifier.hpp"
#include "../src/util/FileUtils.hpp"
#include "../src/util/StringUtils.hpp"
#include "../src/util/HexUtils.hpp"

using namespace JByteDom;
int main() {
    SetConsoleOutputCP(CP_UTF8);

    const char* targetPath = MAKE_STR(PROJECT_RESOURCE_DIR, "Patcher-1.0.jar");
    LOG("目标 Jar 文件路径：", targetPath);

    LOGGER_INDENT_BLOCK(true) {
        auto jarFile = model::jar::JarFile(targetPath);
        if (jarFile.getError()) {
            LOG("构造 Jar 文件时发生错误：", jarFile.getError());
            return -1;
        }
        LOG("总长度：", jarFile.getLen());

        auto jarExtractor = preprocess::extract::JarExtractor(jarFile);
        auto jarEntries = jarExtractor.extractAllEntries();
        if (jarExtractor.getError()) {
            LOG("提取 Jar 内部条目时发生错误：", jarExtractor.getError());
            return -1;
        }
        LOG("内部条目数量：", jarEntries.getSize());
        LOGGER_INDENT_BLOCK(true) {
            auto& entry = jarEntries[5];
            LOG("目标条目路径：", entry.getPath());
            LOGGER_INDENT_BLOCK(true) {
                LOG("名字：", entry.getName());
                LOG("格式：", entry.getExtension());
                LOG("索引：", entry.getIndex());
                LOG("字节数据长度：", entry.getBytesLen());
                LOG("压缩后大小：", entry.getCompressedSize());
                LOG("解压后大小：", entry.getUncompressedSize());
                LOG("压缩方法：", entry.compressionMethodToStr());
                LOG("条目类型：", entry.entryTypeToStr());

                LOGGER_INDENT_BLOCK(true) {
                    u8* bytes = entry.extractBytes(jarFile);
                    if (!bytes || entry.getError()) {
                        LOG("提取目标条目字节数据时发生错误：", entry.getError());
                        return -1;
                    }

                    char* hexStr = util::encode({bytes, entry.getBytesLen()}, (usize) 4);
                    if (hexStr) {
                        LOG("十六进制数据（前 4 字节）：", hexStr);
                        free(hexStr);
                    } else {
                        LOG("无法将目标字节数据转换为十六进制数据");
                    }
                }
            }

            const auto classFiles = preprocess::extract::ClassFileExtractor::extractAllClassFiles(jarFile, jarEntries);
            LOG("提取 Class 文件总数：", classFiles.getSize());
            LOGGER_INDENT_BLOCK(true) {
                const auto& classFile = classFiles[0];
                LOG("目标 Class 文件路径：", classFile.getPath());
                LOGGER_INDENT_BLOCK(true) {
                    LOG("字节数据长度：", classFile.getBytesLen());
                    LOGGER_INDENT_BLOCK(true) {
                        const u8* classFileBytes = classFile.getBytes();
                        char* hexStr = util::encode({classFileBytes, classFile.getBytesLen()}, (usize) 4);
                        if (hexStr) {
                            LOG("十六进制数据（前 4 字节）：", hexStr);
                            free(hexStr);
                        } else {
                            LOG("无法将目标字节数据转换为十六进制数据");
                        }
                    }

                    auto classMetadata = preprocess::parse::ClassFileParser::parseClassFile(classFile);
                    LOG("元数据：");
                    LOGGER_INDENT_BLOCK(true) {
                        LOG("魔法头：", classMetadata.magic);
                        LOG("版本号：", classMetadata.getJavaVersionStr());
                        LOG("类名：", classMetadata.thisClassInternalName);
                        LOG("父类名：", classMetadata.superClassInternalName);
                        LOG("访问标志：", classMetadata.getAccessFlagsStr());
                        LOG("展示全名：", classMetadata.getDisplayFullName());
                        LOG("常量池计数：", classMetadata.constantPoolCount);
                        LOG("接口计数：", classMetadata.interfacesCount);
                        LOG("字段计数：", classMetadata.fieldsCount);
                        LOG("方法计数：", classMetadata.methodsCount);
                        LOG("属性计数：", classMetadata.attributesCount);

                        for (const auto& method : classMetadata.methods) {
                            LOG("目标方法名：", method.internalName, "（常量池索引：", method.nameIndex, "）");
                            LOGGER_INDENT_BLOCK(true) {
                                LOG("签名：", method.descriptor, "（常量池索引：", method.descriptorIndex, "）");
                                LOG("访问标志：", method.getAccessFlagsStr());
                                LOG("展示全名：", method.getDisplayFullName());
                                LOG("最大操作数栈深度：", method.maxStack);
                                LOG("最大局部变量表容量：", method.maxLocals);
                                LOG("方法体字节码总字节数：", method.codeLength);

                                const auto& bytecodes = method.bytecodes;
                                LOG("方法体字节码长度：", bytecodes.getSize());

                                const auto instructions = preprocess::parse::BytecodesParser::parse(bytecodes.peekFirst(), bytecodes.getSize());
                                LOG("方法体指令流长度：", instructions.getSize());

                                LOG("方法体指令流：");
                                LOGGER_INDENT_BLOCK(true) {
                                    for (const auto& instruction : instructions) {
                                        const auto& offset = instruction.offset;
                                        const auto& displayFullName = instruction.getDisplayFullName(classMetadata);
                                        const u32 len = (u32) instruction.operand.length + 1;
                                        LOG("指令（偏移：", offset, "）：", displayFullName, "（长度：", len, "）");
                                    }
                                }
                            }
                        }
                    }

                    auto& methodInfo = classMetadata.methods[2];
                    LOG("目标方法：", methodInfo.getDisplayFullName());
                    LOGGER_INDENT_BLOCK(true) {
                        const auto& bytecodes = methodInfo.bytecodes;
                        LOG("字节码长度：", bytecodes.getSize());

                        const auto instructions = preprocess::parse::BytecodesParser::parse(bytecodes.peekFirst(), bytecodes.getSize());
                        LOG("指令流长度：", instructions.getSize());

                        LOG("指令流：");
                        LOGGER_INDENT_BLOCK(true) {
                            for (const auto& instruction : instructions) {
                                const auto& offset = instruction.offset;
                                const auto& displayFullName = instruction.getDisplayFullName(classMetadata);
                                const u32 len = (u32) instruction.operand.length + 1;
                                LOG("指令（偏移：", offset, "）：", displayFullName, "（长度：", len, "）");
                            }
                        }

                        LOG("插入指令：");
                        LOGGER_INDENT_BLOCK(true) {
                            auto instructionsModifier = runtime::modify::InstructionsModifier(classMetadata, methodInfo);
                            instructionsModifier.insertPrintlnHead("Hello from JByte Dominator!");
                            if (instructionsModifier.applyAllModification()) {
                                LOG("插入成功");
                            } else {
                                LOG("插入失败！");
                            }
                        }

                        const auto& modifierBytecodes = methodInfo.bytecodes;
                        LOG("修改后字节码长度：", modifierBytecodes.getSize());

                        const auto modifierInstructions = preprocess::parse::BytecodesParser::parse(modifierBytecodes.peekFirst(), modifierBytecodes.getSize());
                        LOG("修改后指令流长度：", modifierInstructions.getSize());

                        LOG("修改后指令流：");
                        LOGGER_INDENT_BLOCK(true) {
                            for (const auto& modifierInstruction : modifierInstructions) {
                                const auto& offset = modifierInstruction.offset;
                                const auto& displayFullName = modifierInstruction.getDisplayFullName(classMetadata);
                                const u32 len = (u32) modifierInstruction.operand.length + 1;
                                LOG("指令（偏移：", offset, "）：", displayFullName, "（长度：", len, "）");
                            }
                        }
                    }

                    LOG("序列化元数据：");
                    LOGGER_INDENT_BLOCK(true) {
                        const auto serializedBytes = postprocess::serialize::ClassSerializer::serializeClassMetadata(classMetadata);
                        LOG("序列化后字节数：", serializedBytes.getSize());
                        LOG("原始 Class 文件字节数：", classFile.getBytesLen());

                        const char* internalName = classMetadata.thisClassInternalName;
                        const char* lastSlash = strrchr(internalName, '/');
                        const char* className = lastSlash ? lastSlash + 1 : internalName;
                        char outputPath[512];
                        snprintf(outputPath, sizeof(outputPath), "%s/%s_modified.class", PROJECT_RESOURCE_DIR, className);

                        FILE* outFile = util::openFileUtf8(outputPath, util::FileMode::WriteBinary);
                        if (!outFile) {
                            LOG("无法创建输出文件：", outputPath);
                        } else {
                            usize written = fwrite(serializedBytes.peekFirst(), 1, serializedBytes.getSize(), outFile);
                            fclose(outFile);
                            if (written == serializedBytes.getSize()) {
                                LOG("修改后的 Class 已写入：", outputPath);
                            } else {
                                LOG("写入文件失败（写入字节数不匹配）");
                            }
                        }

                        auto modifiedClassMetadatas = model::common::ElasticArray<model::clazz::ClassMetadata>();
                        modifiedClassMetadatas.push(std::move(classMetadata));

                        char outputJarPath[512];
                        snprintf(outputJarPath, sizeof(outputJarPath), "%s/%s", PROJECT_RESOURCE_DIR, "Patcher-1.0_modified.jar");

                        auto jarPackager = postprocess::package::JarPackager();
                        if (jarPackager.packageJar(targetPath, outputJarPath, modifiedClassMetadatas)) {
                            LOG("JAR 打包成功！输出路径：", outputJarPath);
                        } else {
                            LOG("JAR 打包失败：", jarPackager.getError());
                        }
                    }
                }
            }
        }
    }

    return 0;
}