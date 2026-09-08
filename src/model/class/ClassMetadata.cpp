#include "ClassMetadata.hpp"

#include "../../util/StringUtils.hpp"

namespace JByteDom::model::clazz {
    namespace {
        auto freeClassMetadata(ClassMetadata& classMetadata) -> void {
            // 遍历所有常量池条目，释放所有 Utf8 字符串条目内存并置空，然后销毁所有常量池条目
            for (usize i = 0; i < classMetadata.constantPool.getSize(); ++i) {
                auto& constantPoolEntry = classMetadata.constantPool[i];
                if (constantPoolEntry.tag == ConstantPoolEntryTag::Utf8 && constantPoolEntry.utf8Str) {
                    PTR_FREE_AND_NULL(constantPoolEntry.utf8Str);
                }
            }
            classMetadata.constantPool.clear();

            // 销毁所有缓存的 Utf8 字符串
            classMetadata.utf8Cache.clear();

            // 遍历所有字段，销毁所有字段的所有属性，然后销毁所有方法
            for (usize i = 0; i < classMetadata.fields.getSize(); ++i) {
                auto& fieldInfo = classMetadata.fields[i];
                fieldInfo.attributes.clear();
            }
            classMetadata.fields.clear();

            // 遍历所有方法，销毁所有方法的所有属性、所有方法体属性、所有方法体字节码属性，释放所有方法的限定名内存并置空，然后销毁所有方法
            for (usize i = 0; i < classMetadata.methods.getSize(); ++i) {
                auto& methodInfo = classMetadata.methods[i];
                methodInfo.attributes.clear();
                methodInfo.codeAttributes.clear();
                methodInfo.bytecodes.clear();
                PTR_FREE_AND_NULL(methodInfo.qualifiedName);
            }
            classMetadata.methods.clear();

            // 遍历所有属性，销毁所有属性的所有属性字节数据，然后销毁所有属性
            for (usize i = 0; i < classMetadata.attributes.getSize(); ++i) {
                auto& attributeInfo = classMetadata.attributes[i];
                attributeInfo.attributeBytes.clear();
            }
            classMetadata.attributes.clear();

            // 销毁所有接口
            classMetadata.interfaces.clear();

            // 释放类限定名和父类限定名并置空
            PTR_FREE_AND_NULL(classMetadata.thisClassQualifiedName);
            PTR_FREE_AND_NULL(classMetadata.superClassQualifiedName);

            // 清空常量池条目、字段、方法、属性、接口的计数
            classMetadata.constantPoolCount = 0;
            classMetadata.fieldsCount = 0;
            classMetadata.methodsCount = 0;
            classMetadata.attributesCount = 0;
            classMetadata.interfacesCount = 0;
        }
    }

    namespace {
        thread_local char* dynamicBuf = nullptr;
        thread_local usize dynamicCap = 0;
        auto descriptorToTypeName(const char* descriptor) -> const char* {
            if (!descriptor || descriptor[0] == '\0') {
                // 确保缓冲区至少能装下 "void"
                if (dynamicCap < 5) {
                    const auto newBuf = (char*) realloc(dynamicBuf, 5);
                    if (!newBuf) {
                        return "[Error]";
                    }
                    dynamicBuf = newBuf;
                    dynamicCap = 5;
                }

                strcpy(dynamicBuf, "void");

                return dynamicBuf;
            }

            {
                const usize estimatedCap = strlen(descriptor) * 2 + 16;
                if (dynamicCap < estimatedCap) {
                    const auto newBuf = (char*) realloc(dynamicBuf, estimatedCap);
                    if (!newBuf) {
                        return "[Error]";
                    }
                    dynamicBuf = newBuf;
                    dynamicCap = estimatedCap;
                }
            }

            char* buf = dynamicBuf;
            buf[0] = '\0';

            if (descriptor[0] == '[') {
                // 递归获取内部类型
                const char* inner = descriptorToTypeName(descriptor + 1);
                const usize innerLen = strlen(inner);

                // 动态分配临时缓冲区
                const auto tempBuf = (char*) malloc(innerLen + 1);
                if (!tempBuf) {
                    return "[Error]";
                }

                // 把内部类型拷贝出来
                memcpy(tempBuf, inner, innerLen + 1);

                // 确保 dynamicBuf 能装下 "内部类型 + []"
                const usize estimatedCap = innerLen + 2 + 1;
                if (dynamicCap < estimatedCap) {
                    const auto newBuf = (char*) realloc(dynamicBuf, estimatedCap);
                    if (!newBuf) {
                        free(tempBuf);
                        return "[Error]";
                    }
                    dynamicBuf = newBuf;
                    dynamicCap = estimatedCap;
                }

                // 现在源和目标完全分离，安全拼接
                snprintf(dynamicBuf, dynamicCap, "%s[]", tempBuf);

                free(tempBuf);

                return dynamicBuf;
            }

            if (descriptor[0] == 'L') {
                char* writePtr = buf;

                const char* ptr = descriptor + 1;
                while (*ptr && *ptr != ';') {
                    *writePtr++ = *ptr == '/' ? '.' : *ptr;
                    ptr++;
                }

                *writePtr = '\0';

                return buf;
            }

            const char* typeName;
            switch (descriptor[0]) {
                case 'B': typeName = "byte"; break;
                case 'C': typeName = "char"; break;
                case 'D': typeName = "double"; break;
                case 'F': typeName = "float"; break;
                case 'I': typeName = "int"; break;
                case 'J': typeName = "long"; break;
                case 'S': typeName = "short"; break;
                case 'Z': typeName = "boolean"; break;
                case 'V': typeName = "void"; break;
                default: typeName = "unknown"; break;
            }

            strcpy(buf, typeName);

            return buf;
        }
    }

    auto FieldInfo::getAccessFlagsStr() const -> const char* {
        thread_local char buf[96];
        buf[0] = '\0';

        if (this->accessFlags & AccessFlag::Field::Public) strcat(buf, "public ");
        if (this->accessFlags & AccessFlag::Field::Private) strcat(buf, "private ");
        if (this->accessFlags & AccessFlag::Field::Protected) strcat(buf, "protected ");
        if (this->accessFlags & AccessFlag::Field::Static) strcat(buf, "static ");
        if (this->accessFlags & AccessFlag::Field::Final) strcat(buf, "final ");
        if (this->accessFlags & AccessFlag::Field::Volatile) strcat(buf, "volatile ");
        if (this->accessFlags & AccessFlag::Field::Transient) strcat(buf, "transient ");
        if (this->accessFlags & AccessFlag::Field::Synthetic) strcat(buf, "synthetic ");
        if (this->accessFlags & AccessFlag::Field::Enum) strcat(buf, "enum ");

        if (buf[0] != '\0') {
            buf[strlen(buf) - 1] = '\0';
        }

        return buf;
    }
    auto FieldInfo::getDisplayFullName() const -> const char* {
        thread_local char buf[1024];
        buf[0] = '\0';

        const char* accessFlagsStr = getAccessFlagsStr();
        if (accessFlagsStr && accessFlagsStr[0] != '\0' && strcmp(accessFlagsStr, "default") != 0) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", accessFlagsStr);
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " ");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", descriptorToTypeName(this->descriptor));

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " ");
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", this->name ? this->name : "<unnamed>");

        return buf;
    }

    auto MethodInfo::getAccessFlagsStr() const -> const char * {
        static char buf[160];
        buf[0] = '\0';

        if (this->accessFlags & AccessFlag::Method::Public) strcat(buf, "public ");
        if (this->accessFlags & AccessFlag::Method::Private) strcat(buf, "private ");
        if (this->accessFlags & AccessFlag::Method::Protected) strcat(buf, "protected ");
        if (this->accessFlags & AccessFlag::Method::Static) strcat(buf, "static ");
        if (this->accessFlags & AccessFlag::Method::Final) strcat(buf, "final ");
        if (this->accessFlags & AccessFlag::Method::Synchronized) strcat(buf, "synchronized ");
        if (this->accessFlags & AccessFlag::Method::Bridge) strcat(buf, "bridge ");
        if (this->accessFlags & AccessFlag::Method::Varargs) strcat(buf, "varargs ");
        if (this->accessFlags & AccessFlag::Method::Native) strcat(buf, "native ");
        if (this->accessFlags & AccessFlag::Method::Abstract) strcat(buf, "abstract ");
        if (this->accessFlags & AccessFlag::Method::Strictfp) strcat(buf, "strictfp ");
        if (this->accessFlags & AccessFlag::Method::Synthetic) strcat(buf, "synthetic ");

        if (buf[0] != '\0') {
            buf[strlen(buf) - 1] = '\0';
        }

        return buf;
    }
    auto MethodInfo::getDisplayFullName() const -> const char* {
        thread_local char buf[4096];
        buf[0] = '\0';

        const char* accessFlagsStr = getAccessFlagsStr();
        if (accessFlagsStr && accessFlagsStr[0] != '\0' && strcmp(accessFlagsStr, "default") != 0) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", accessFlagsStr);
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " ");
        }

        if (this->descriptor) {
            const char* parenPos = strchr(this->descriptor, ')');
            if (parenPos) {
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", descriptorToTypeName(parenPos + 1));
            } else {
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "void");
            }
        } else {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "void");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " ");
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", this->qualifiedName ? this->qualifiedName : "<unnamed>");

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "(");
        if (this->descriptor && this->descriptor[0] == '(') {
            const char* p = this->descriptor + 1;
            bool first = true;
            while (*p && *p != ')') {
                const char* start = p;

                while (*p == '[') {
                    p++;
                }

                if (*p == 'L') {
                    while (*p && *p != ';') p++;
                    if (*p == ';') p++;
                } else {
                    p++;
                }

                if (!first) {
                    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", ", ");
                }

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", descriptorToTypeName(start));

                first = false;
            }
        }
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", ")");

        return buf;
    }

    ClassMetadata::~ClassMetadata() {
        freeClassMetadata(*this);
    }

    ClassMetadata::ClassMetadata(ClassMetadata&& other) noexcept:
        magic(other.magic),
        minorVersion(other.minorVersion),
        majorVersion(other.majorVersion),
        constantPoolCount(other.constantPoolCount),
        constantPool(std::move(other.constantPool)),
        accessFlags(other.accessFlags),
        thisClass(other.thisClass),
        superClass(other.superClass),
        interfacesCount(other.interfacesCount),
        interfaces(std::move(other.interfaces)),
        fieldsCount(other.fieldsCount),
        fields(std::move(other.fields)),
        methodsCount(other.methodsCount),
        methods(std::move(other.methods)),
        attributesCount(other.attributesCount),
        attributes(std::move(other.attributes)),
        thisClassInternalName(other.thisClassInternalName),
        thisClassQualifiedName(other.thisClassQualifiedName),
        superClassInternalName(other.superClassInternalName),
        superClassQualifiedName(other.superClassQualifiedName),
        utf8Cache(std::move(other.utf8Cache))
    {
        other.magic = 0;
        other.minorVersion = 0;
        other.majorVersion = 0;
        other.constantPoolCount = 0;
        other.accessFlags = 0;
        other.thisClass = 0;
        other.superClass = 0;
        other.interfacesCount = 0;
        other.fieldsCount = 0;
        other.methodsCount = 0;
        other.attributesCount = 0;
        other.thisClassInternalName = nullptr;
        other.thisClassQualifiedName = nullptr;
        other.superClassInternalName = nullptr;
        other.superClassQualifiedName = nullptr;
    }
    ClassMetadata& ClassMetadata::operator = (ClassMetadata&& other) noexcept {
        if (this != &other) {
            freeClassMetadata(*this);

            this->magic = other.magic;
            this->minorVersion = other.minorVersion;
            this->majorVersion = other.majorVersion;
            this->constantPoolCount = other.constantPoolCount;
            this->constantPool = std::move(other.constantPool);
            this->accessFlags = other.accessFlags;
            this->thisClass = other.thisClass;
            this->superClass = other.superClass;
            this->interfacesCount = other.interfacesCount;
            this->interfaces = std::move(other.interfaces);
            this->fieldsCount = other.fieldsCount;
            this->fields = std::move(other.fields);
            this->methodsCount = other.methodsCount;
            this->methods = std::move(other.methods);
            this->attributesCount = other.attributesCount;
            this->attributes = std::move(other.attributes);
            this->thisClassInternalName = other.thisClassInternalName;
            this->thisClassQualifiedName = other.thisClassQualifiedName;
            this->superClassInternalName = other.superClassInternalName;
            this->superClassQualifiedName = other.superClassQualifiedName;
            this->utf8Cache = std::move(other.utf8Cache);

            other.magic = 0;
            other.minorVersion = 0;
            other.majorVersion = 0;
            other.constantPoolCount = 0;
            other.accessFlags = 0;
            other.thisClass = 0;
            other.superClass = 0;
            other.interfacesCount = 0;
            other.fieldsCount = 0;
            other.methodsCount = 0;
            other.attributesCount = 0;
            other.thisClassInternalName = nullptr;
            other.thisClassQualifiedName = nullptr;
            other.superClassInternalName = nullptr;
            other.superClassQualifiedName = nullptr;
        }

        return *this;
    }

    auto ClassMetadata::getJavaVersionStr() const -> const char* {
        const char* releaseName = nullptr;
        switch (this->majorVersion) {
            case 45: releaseName = "Java 1.1"; break;
            case 46: releaseName = "Java 1.2"; break;
            case 47: releaseName = "Java 1.3"; break;
            case 48: releaseName = "Java 1.4"; break;
            case 49: releaseName = "Java 5"; break;
            case 50: releaseName = "Java 6"; break;
            case 51: releaseName = "Java 7"; break;
            case 52: releaseName = "Java 8"; break;
            case 53: releaseName = "Java 9"; break;
            case 54: releaseName = "Java 10"; break;
            case 55: releaseName = "Java 11"; break;
            case 56: releaseName = "Java 12"; break;
            case 57: releaseName = "Java 13"; break;
            case 58: releaseName = "Java 14"; break;
            case 59: releaseName = "Java 15"; break;
            case 60: releaseName = "Java 16"; break;
            case 61: releaseName = "Java 17"; break;
            case 62: releaseName = "Java 18"; break;
            case 63: releaseName = "Java 19"; break;
            case 64: releaseName = "Java 20"; break;
            case 65: releaseName = "Java 21"; break;
            case 66: releaseName = "Java 22"; break;
            case 67: releaseName = "Java 23"; break;
            case 68: releaseName = "Java 24"; break;
            case 69: releaseName = "Java 25"; break;
            case 70: releaseName = "Java 26"; break;
            case 71: releaseName = "Java 27"; break;
            case 72: releaseName = "Java 28"; break;
            case 73: releaseName = "Java 29"; break;
            case 74: releaseName = "Java 30"; break;
            default: releaseName = "Unknown"; break;
        }

        static char buf[64];
        if (this->minorVersion == 0) {
            snprintf(buf, sizeof(buf), "%d (%s)", this->majorVersion, releaseName);
        } else {
            snprintf(buf, sizeof(buf), "%d.%d (%s)", this->majorVersion, this->minorVersion, releaseName);
        }

        return buf;
    }
    auto ClassMetadata::getAccessFlagsStr() const -> const char* {
        thread_local char buf[96];
        buf[0] = '\0';

        if (this->accessFlags & AccessFlag::Class::Public) strcat(buf, "public ");
        if (this->accessFlags & AccessFlag::Class::Final) strcat(buf, "final ");
        if (this->accessFlags & AccessFlag::Class::Interface) strcat(buf, "interface ");
        if (this->accessFlags & AccessFlag::Class::Abstract) strcat(buf, "abstract ");
        if (this->accessFlags & AccessFlag::Class::Annotation) strcat(buf, "@annotation ");
        if (this->accessFlags & AccessFlag::Class::Enum) strcat(buf, "enum ");
        if (this->accessFlags & AccessFlag::Class::Synthetic) strcat(buf, "synthetic ");
        if (this->accessFlags & AccessFlag::Class::Module) strcat(buf, "module ");

        if (buf[0] != '\0') {
            buf[strlen(buf) - 1] = '\0';
        }

        return buf;
    }
    auto ClassMetadata::getDisplayFullName() const -> const char* {
        thread_local char buf[2048];
        buf[0] = '\0';

        const char* accessFlagsStr = getAccessFlagsStr();
        if (accessFlagsStr && accessFlagsStr[0] != '\0' && strcmp(accessFlagsStr, "default") != 0) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", accessFlagsStr);
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " ");
        }

        if (this->accessFlags & AccessFlag::Class::Interface) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "interface ");
        } else if (this->accessFlags & AccessFlag::Class::Enum) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "enum ");
        } else if (this->accessFlags & AccessFlag::Class::Annotation) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "@interface ");
        } else {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", "class ");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", this->thisClassQualifiedName ? this->thisClassQualifiedName : "<unknown>");

        if (this->superClassQualifiedName && strcmp(this->superClassQualifiedName, "java.lang.Object") != 0) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", " extends ");
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", this->superClassQualifiedName);
        }

        return buf;
    }

    auto ClassMetadata::findFieldByName(const char* name) const -> const FieldInfo* {
        if (!name) {
            return nullptr;
        }

        for (usize i = 0; i < this->fields.getSize(); ++i) {
            const auto& field = this->fields[i];
            if (field.name && strcmp(field.name, name) == 0) {
                return &field;
            }
        }

        return nullptr;
    }

    auto ClassMetadata::findMethodsByInternalName(const char* internalName) const -> common::ElasticArray<const MethodInfo*> {
        auto methodInfos = common::ElasticArray<const MethodInfo*>();

        if (!internalName) {
            return methodInfos;
        }

        for (usize i = 0; i < this->methods.getSize(); ++i) {
            const auto& method = this->methods[i];
            if (method.internalName && strcmp(method.internalName, internalName) == 0) {
                methodInfos.push(&method);
            }
        }

        return methodInfos;
    }
    auto ClassMetadata::findMethodsByQualifiedName(const char* qualifiedName) const -> common::ElasticArray<const MethodInfo*> {
        auto methodInfos = common::ElasticArray<const MethodInfo*>();

        if (!qualifiedName) {
            return methodInfos;
        }

        for (usize i = 0; i < this->methods.getSize(); ++i) {
            const auto& method = this->methods[i];
            if (method.qualifiedName && strcmp(method.qualifiedName, qualifiedName) == 0) {
                methodInfos.push(&method);
            }
        }

        return methodInfos;
    }

    auto ClassMetadata::addUtf8Entry(const char* str) -> u16 {
        for (usize i = 1; i < this->constantPool.getSize(); ++i) {
            const auto& entry = this->constantPool[i];
            if (entry.tag == ConstantPoolEntryTag::Utf8 && entry.utf8Str && strcmp(entry.utf8Str, str) == 0) {
                return (u16) i;
            }
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::Utf8;
        entry.utf8Len = (u16) strlen(str);
        entry.utf8Str = (char*) malloc(entry.utf8Len + 1);
        if (!entry.utf8Str) {
            return 0;
        }
        memcpy(entry.utf8Str, str, entry.utf8Len + 1);

        const u16 newIndex = (u16) this->constantPool.getSize();
        const char* utf8Ptr = entry.utf8Str;
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(utf8Ptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();
        return newIndex;
    }
    auto ClassMetadata::addClassEntry(const char* internalName) -> u16 {
        const u16 nameIndex = addUtf8Entry(internalName);
        if (nameIndex == 0) {
            return 0;
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::Class;
        entry.nameIndex = nameIndex;

        const u16 newIndex = (u16) this->constantPool.getSize();
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(nullptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();

        return newIndex;
    }
    auto ClassMetadata::addNameAndTypeEntry(const char* name, const char* descriptor) -> u16 {
        const u16 nameIndex = addUtf8Entry(name);
        const u16 descriptorIndex = addUtf8Entry(descriptor);
        if (nameIndex == 0 || descriptorIndex == 0) {
            return 0;
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::NameAndType;
        entry.nameIndex = nameIndex;
        entry.descriptorIndex = descriptorIndex;

        const u16 newIndex = (u16) this->constantPool.getSize();
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(nullptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();
        return newIndex;
    }
    auto ClassMetadata::addFieldrefEntry(const char* className, const char* fieldName, const char* descriptor) -> u16 {
        const u16 classIndex = addClassEntry(className);
        const u16 nameAndTypeIndex = addNameAndTypeEntry(fieldName, descriptor);
        if (classIndex == 0 || nameAndTypeIndex == 0) {
            return 0;
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::Fieldref;
        entry.classIndex = classIndex;
        entry.nameAndTypeIndex = nameAndTypeIndex;

        const u16 newIndex = (u16) this->constantPool.getSize();
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(nullptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();
        return newIndex;
    }
    auto ClassMetadata::addMethodrefEntry(const char* className, const char* methodName, const char* descriptor) -> u16 {
        const u16 classIndex = addClassEntry(className);
        const u16 nameAndTypeIndex = addNameAndTypeEntry(methodName, descriptor);
        if (classIndex == 0 || nameAndTypeIndex == 0) {
            return 0;
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::Methodref;
        entry.classIndex = classIndex;
        entry.nameAndTypeIndex = nameAndTypeIndex;

        const u16 newIndex = (u16) this->constantPool.getSize();
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(nullptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();
        return newIndex;
    }
    auto ClassMetadata::addStringEntry(const char* str) -> u16 {
        const u16 utf8Index = addUtf8Entry(str);
        if (utf8Index == 0) {
            return 0;
        }

        auto entry = ConstantPoolEntry{};
        entry.tag = ConstantPoolEntryTag::String;
        entry.nameIndex = utf8Index;

        const u16 newIndex = (u16) this->constantPool.getSize();
        this->constantPool.push(std::move(entry));
        this->utf8Cache.push(nullptr);
        this->constantPoolCount = (u16) this->constantPool.getSize();
        return newIndex;
    }
}
