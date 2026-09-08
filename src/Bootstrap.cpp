// 本文件，即本项目的 UI 部分由 AI 辅助生成。
// 作者本人专注于后端核心逻辑（字节码解析、修改、JAR 打包等），
// 对 GUI 布局和交互设计不太擅长，因此 UI 代码可能不够优雅，
// 但功能完整可用。欢迎提 issue 改进 UI 体验。

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Menu_Item.H>

#include <cstdio>
#include <cstring>
#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <windows.h>

#include "model/jar/JarFile.hpp"
#include "model/jar/JarEntry.hpp"
#include "preprocess/extract/JarExtractor.hpp"
#include "preprocess/extract/ClassFileExtractor.hpp"
#include "preprocess/parse/ClassFileParser.hpp"
#include "preprocess/parse/BytecodesParser.hpp"
#include "model/class/ClassMetadata.hpp"
#include "model/common/ElasticArray.hpp"
#include "model/bytecode/Instruction.hpp"
#include "runtime/modify/InstructionsModifier.hpp"
#include "postprocess/package/JarPackager.hpp"

using namespace JByteDom;

// ─── 解码 Modified UTF-8 → 标准 UTF-8 ──────────────────────────

static std::string decodeModifiedUTF8(const u8* bytes, usize len) {
    if (!bytes || len == 0) return "";

    usize printableCount = 0;
    for (usize i = 0; i < len; ++i) {
        u8 c = bytes[i];
        if (c >= 0x20 && c <= 0x7E) ++printableCount;
    }

    bool isBinary = (printableCount < len / 2);
    std::string result;
    result.reserve(len * 2);

    if (isBinary) {
        result += "[Binary Data, Length: " + std::to_string(len) + "] -> ";
        for (usize i = 0; i < len; ++i) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\x%02X", bytes[i]);
            result += buf;
        }
        return result;
    }

    usize i = 0;
    while (i < len) {
        u8 c = bytes[i];

        if (c == 0xC0 && i + 1 < len && bytes[i + 1] == 0x80) {
            result += "\\0";
            i += 2;
            continue;
        }

        if (c == 0xED && i + 5 < len) {
            uint32_t high = ((c & 0x0F) << 12) | ((bytes[i+1] & 0x3F) << 6) | (bytes[i+2] & 0x3F);
            uint32_t low  = ((bytes[i+3] & 0x0F) << 12) | ((bytes[i+4] & 0x3F) << 6) | (bytes[i+5] & 0x3F);
            if (high >= 0xD800 && high <= 0xDBFF && low >= 0xDC00 && low <= 0xDFFF) {
                uint32_t codepoint = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                result.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
                result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                i += 6;
                continue;
            }
        }

        if (c < 0x80) {
            if (c >= 0x20 && c <= 0x7E) {
                result.push_back(static_cast<char>(c));
            } else {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\x%02X", c);
                result += buf;
            }
            i += 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < len) {
            uint32_t cp = ((c & 0x1F) << 6) | (bytes[i+1] & 0x3F);
            if (cp >= 0x20 && cp <= 0x7E) {
                result.push_back(static_cast<char>(cp));
            } else if (cp == 0) {
                result += "\\0";
            } else {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\x%02X", cp);
                result += buf;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < len) {
            result.push_back(static_cast<char>(c));
            result.push_back(static_cast<char>(bytes[i+1]));
            result.push_back(static_cast<char>(bytes[i+2]));
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < len) {
            result.push_back(static_cast<char>(c));
            result.push_back(static_cast<char>(bytes[i+1]));
            result.push_back(static_cast<char>(bytes[i+2]));
            result.push_back(static_cast<char>(bytes[i+3]));
            i += 4;
        } else {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\x%02X", c);
            result += buf;
            i += 1;
        }
    }

    return result;
}

// ─── 自定义 UI 控件 ─────────────────────────────────────────────

class CustomTree : public Fl_Tree {
public:
    CustomTree(int x, int y, int w, int h, const char* l = nullptr)
        : Fl_Tree(x, y, w, h, l) {
        apply_dark_scrollbars();
    }

    void apply_dark_scrollbars() const {
        for (int i = 0; i < children(); ++i) {
            Fl_Widget* child_widget = child(i);
            if (auto* sb = dynamic_cast<Fl_Scrollbar*>(child_widget)) {
                sb->box(FL_FLAT_BOX);
                sb->color(fl_rgb_color(35, 35, 35));
                sb->selection_color(fl_rgb_color(70, 70, 70));
            }
        }
    }
};

class ClickableTextDisplay : public Fl_Text_Display {
public:
    ClickableTextDisplay(int x, int y, int w, int h, const char* l = nullptr)
        : Fl_Text_Display(x, y, w, h, l) {
        if (mVScrollBar) {
            mVScrollBar->box(FL_FLAT_BOX);
            mVScrollBar->color(fl_rgb_color(35, 35, 35));
            mVScrollBar->selection_color(fl_rgb_color(70, 70, 70));
        }
        if (mHScrollBar) {
            mHScrollBar->box(FL_FLAT_BOX);
            mHScrollBar->color(fl_rgb_color(35, 35, 35));
            mHScrollBar->selection_color(fl_rgb_color(70, 70, 70));
        }
    }

    int last_line_start = 0;
    int last_line_end = 0;

    void handle_click_callback(std::function<void()> cb) {
        click_callback = std::move(cb);
    }

    void handle_right_click_callback(std::function<void(int lineStart)> cb) {
        right_click_callback = std::move(cb);
    }

    int handle(int event) override {
        if (event == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
            int x = Fl::event_x();
            int y = Fl::event_y();
            int pos = xy_to_position(x, y);
            if (pos >= 0 && buffer()) {
                int lineStart = buffer()->line_start(pos);
                if (right_click_callback) {
                    right_click_callback(lineStart);
                    return 1;
                }
            }
            return Fl_Text_Display::handle(event);
        }

        if (event == FL_RELEASE && buffer() && Fl::event_button() == FL_LEFT_MOUSE) {
            int x = Fl::event_x();
            int y = Fl::event_y();
            int pos = xy_to_position(x, y);
            if (pos < 0) return Fl_Text_Display::handle(event);
            last_line_start = buffer()->line_start(pos);
            last_line_end = buffer()->line_end(pos);
            buffer()->select(last_line_start, last_line_end);
            if (click_callback) click_callback();
            return 1;
        }

        return Fl_Text_Display::handle(event);
    }

private:
    std::function<void()> click_callback;
    std::function<void(int)> right_click_callback;
};

class HoverLabel : public Fl_Box {
public:
    HoverLabel(int x, int y, int w, int h, const char* l = nullptr)
        : Fl_Box(x, y, w, h, l) {
        box(FL_FLAT_BOX);
        labelcolor(fl_rgb_color(200, 200, 200));
        align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
        labelsize(12);
        color(fl_rgb_color(43, 43, 43));
    }

    int handle(int event) override {
        switch (event) {
            case FL_ENTER:
            case FL_MOVE:
            case FL_LEAVE:
                return 1;
            default:
                return Fl_Box::handle(event);
        }
    }
};

// ─── 全局应用状态 ──────────────────────────────────────────────

struct AppData {
    model::common::ElasticArray<model::clazz::ClassMetadata> classMetadatas;
    model::common::ElasticArray<model::jar::JarEntry> jarEntries;
    int currentClassIndex = -1;
    const model::clazz::ClassMetadata* currentMetadata = nullptr;
    model::jar::JarFile* currentJarFile = nullptr;

    CustomTree* classTree = nullptr;
    ClickableTextDisplay* structDisplay = nullptr;
    Fl_Text_Buffer* structBuffer = nullptr;
    ClickableTextDisplay* insnDisplay = nullptr;
    Fl_Text_Buffer* insnBuffer = nullptr;

    HoverLabel* fileLabel = nullptr;
    std::string fullJarPath;
    bool showConstantPool = false;
    bool showFields = true;
    bool showMethods = true;

    Fl_Check_Button* cpCheck = nullptr;
    Fl_Check_Button* fieldCheck = nullptr;
    Fl_Check_Button* methodCheck = nullptr;

    std::unordered_map<int, usize> insnLineToIndex;
    usize selectedMethodIndex = 0;
    usize currentInsnIndex = 0;

    std::atomic<bool> isPacking{false};

    Fl_Box* statusLabel = nullptr;  // 状态标签
};

AppData g_appData;

// ─── 工具函数 ──────────────────────────────────────────────────

void closeAllTreeItems(Fl_Tree_Item* item) {
    if (!item) return;
    item->close();
    for (int i = 0; i < item->children(); ++i) closeAllTreeItems(item->child(i));
}

// ─── 状态标签定时恢复 ─────────────────────────────────────────

static void resetStatusLabel(void*) {
    if (g_appData.statusLabel) {
        g_appData.statusLabel->label("就绪");
        g_appData.statusLabel->redraw();
    }
}

// ─── 渲染函数 ──────────────────────────────────────────────────

std::string renderClassStructure(const model::clazz::ClassMetadata& md) {
    std::string out;

    std::string className = md.thisClassQualifiedName ?
        decodeModifiedUTF8(reinterpret_cast<const u8*>(md.thisClassQualifiedName), strlen(md.thisClassQualifiedName)) : "<unknown>";
    std::string superName = md.superClassQualifiedName ?
        decodeModifiedUTF8(reinterpret_cast<const u8*>(md.superClassQualifiedName), strlen(md.superClassQualifiedName)) : "<none>";

    out += "──── Class: " + className +
        (superName.empty() || superName == "java.lang.Object" ? "" : " extends " + superName) +
        " ────\n\n";

    out += "Magic: 0x" + std::to_string(md.magic) + "\n";
    out += "Version: " + std::string(md.getJavaVersionStr()) + "\n";
    out += "Access Flags: " + std::string(md.getAccessFlagsStr()) + "\n";
    out += "Super Class: " + superName + "\n";

    if (g_appData.showConstantPool) {
        out += "\n──── 常量池 (" + std::to_string(md.constantPoolCount) + " entries) ────\n";
        for (usize i = 0; i < md.constantPool.getSize(); ++i) {
            const auto& entry = md.constantPool[i];
            char buf[128];
            snprintf(buf, sizeof(buf), "#%zu  tag=%d", i, entry.tag);
            out += buf;

            if (entry.tag == 1 && entry.utf8Str) {
                std::string decoded = decodeModifiedUTF8(
                    reinterpret_cast<const u8*>(entry.utf8Str),
                    entry.utf8Len
                );
                out += "  \"" + decoded + "\"";
            } else {
                switch (entry.tag) {
                    case 3:  out += "  [Integer]"; break;
                    case 4:  out += "  [Float]"; break;
                    case 5:  out += "  [Long]"; break;
                    case 6:  out += "  [Double]"; break;
                    case 7:  out += "  [Class]"; break;
                    case 8:  out += "  [String]"; break;
                    case 9:  out += "  [Fieldref]"; break;
                    case 10: out += "  [Methodref]"; break;
                    case 11: out += "  [InterfaceMethodref]"; break;
                    case 12: out += "  [NameAndType]"; break;
                    case 15: out += "  [MethodHandle]"; break;
                    case 16: out += "  [MethodType]"; break;
                    case 18: out += "  [InvokeDynamic]"; break;
                    default: out += "  [Other]"; break;
                }
            }
            out += "\n";
        }
        out += "\n";
    } else {
        out += "\n──── 常量池 (已折叠) ────\n\n";
    }

    if (g_appData.showFields) {
        out += "──── 字段 (" + std::to_string(md.fieldsCount) + ") ────\n";
        for (usize i = 0; i < md.fields.getSize(); ++i) {
            const auto& field = md.fields[i];
            out += std::string(field.getDisplayFullName()) + "\n";
        }
        out += "\n";
    } else {
        out += "──── 字段 (已折叠) ────\n\n";
    }

    if (g_appData.showMethods) {
        out += "──── 方法 (" + std::to_string(md.methodsCount) + ") ────\n";
        out += "（点击下面方法行，在下方查看指令流）\n";
        for (usize i = 0; i < md.methods.getSize(); ++i) {
            const auto& method = md.methods[i];
            out += std::string(method.getDisplayFullName());
            if (method.hasCode) out += " [" + std::to_string(method.codeLength) + " bytes]";
            out += "\n";
        }
    } else {
        out += "──── 方法 (已折叠) ────\n";
    }

    return out;
}

std::string renderInstructions(const model::clazz::ClassMetadata& md, const model::common::ElasticArray<u8>& bytecodes) {
    auto instructions = preprocess::parse::BytecodesParser::parse(bytecodes.peekFirst(), bytecodes.getSize());
    std::string out;
    out += "──── 指令流 (" + std::to_string(instructions.getSize()) + " insns) ────\n\n";

    g_appData.insnLineToIndex.clear();

    for (usize i = 0; i < instructions.getSize(); ++i) {
        const auto& insn = instructions[i];
        const char* displayName = insn.getDisplayFullName(md);
        char buf[256];
        int pos = out.size();
        snprintf(buf, sizeof(buf), "%04X:  %s\n", insn.offset, displayName ? displayName : "<unknown>");
        out += buf;
        g_appData.insnLineToIndex[pos] = i;
    }
    return out;
}

// ─── 回调函数 ──────────────────────────────────────────────────

void updateStructDisplay() {
    if (!g_appData.currentMetadata) return;
    std::string structText = renderClassStructure(*g_appData.currentMetadata);
    g_appData.structBuffer->text(structText.c_str());
    g_appData.structDisplay->redraw();
}

void onStructClicked(Fl_Widget* widget, void* data) {
    auto display = (ClickableTextDisplay*)widget;
    int lineStart = display->last_line_start;
    int lineEnd = display->last_line_end;
    std::string line(g_appData.structBuffer->text() + lineStart, lineEnd - lineStart);

    if (!g_appData.currentMetadata) {
        g_appData.insnBuffer->text("（请先选择一个类）");
        g_appData.insnDisplay->redraw();
        return;
    }

    const auto& md = *g_appData.currentMetadata;
    for (usize i = 0; i < md.methods.getSize(); ++i) {
        const auto& method = md.methods[i];
        const char* displayName = method.getDisplayFullName();
        if (displayName && line.find(displayName) != std::string::npos) {
            g_appData.selectedMethodIndex = i;
            if (method.hasCode) {
                std::string insnText = renderInstructions(md, method.bytecodes);
                g_appData.insnBuffer->text(insnText.c_str());
            } else {
                g_appData.insnBuffer->text("（该方法没有字节码，可能是 native 或 abstract）");
                g_appData.insnLineToIndex.clear();
            }
            g_appData.insnDisplay->redraw();
            return;
        }
    }

    g_appData.insnBuffer->text("（请点击方法行查看指令流）");
    g_appData.insnDisplay->redraw();
}

void onToggleView(Fl_Widget* widget, void* data) {
    if (widget == g_appData.cpCheck) g_appData.showConstantPool = (g_appData.cpCheck->value() > 0);
    else if (widget == g_appData.fieldCheck) g_appData.showFields = (g_appData.fieldCheck->value() > 0);
    else if (widget == g_appData.methodCheck) g_appData.showMethods = (g_appData.methodCheck->value() > 0);
    updateStructDisplay();
}

void onClassSelected(Fl_Widget* widget, void* data) {
    auto tree = (Fl_Tree*)widget;
    auto item = tree->callback_item();
    if (!item) item = tree->item_clicked();
    if (!item) return;

    if (Fl::event_clicks() > 0 && item->children() > 0) {
        if (item->is_open()) item->close();
        else item->open();
        tree->redraw();
        return;
    }

    char fullPathBuf[512];
    tree->item_pathname(fullPathBuf, sizeof(fullPathBuf), item);

    for (usize i = 0; i < g_appData.classMetadatas.getSize(); ++i) {
        const auto& md = g_appData.classMetadatas[i];
        if (md.thisClassInternalName && strcmp(md.thisClassInternalName, fullPathBuf) == 0) {
            g_appData.currentMetadata = &md;
            g_appData.currentClassIndex = i;
            g_appData.selectedMethodIndex = 0;
            g_appData.currentInsnIndex = 0;
            updateStructDisplay();
            g_appData.insnBuffer->text("");
            g_appData.insnDisplay->redraw();
            return;
        }
    }
}

void reloadJar(const char* jarPath) {
    if (g_appData.currentJarFile) {
        delete g_appData.currentJarFile;
        g_appData.currentJarFile = nullptr;
    }

    g_appData.jarEntries.clear();
    g_appData.classMetadatas.clear();
    g_appData.currentMetadata = nullptr;
    g_appData.currentClassIndex = -1;
    g_appData.selectedMethodIndex = 0;
    g_appData.currentInsnIndex = 0;

    if (g_appData.classTree) g_appData.classTree->clear();
    if (g_appData.structBuffer) g_appData.structBuffer->text("");
    if (g_appData.insnBuffer) g_appData.insnBuffer->text("");

    auto* jarFile = new model::jar::JarFile(jarPath);
    if (jarFile->getError()) {
        printf("Error: %s\n", jarFile->getError());
        delete jarFile;
        return;
    }

    g_appData.currentJarFile = jarFile;
    g_appData.fullJarPath = jarPath;

    std::string pathStr = jarPath;
    usize lastSlash = pathStr.find_last_of("/\\");
    std::string fileName = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;

    if (g_appData.fileLabel) {
        g_appData.fileLabel->label(fileName.c_str());
        g_appData.fileLabel->tooltip(g_appData.fullJarPath.c_str());
    }

    auto jarExtractor = preprocess::extract::JarExtractor(*jarFile);
    g_appData.jarEntries = jarExtractor.extractAllEntries();
    if (jarExtractor.getError()) {
        printf("Extract error: %s\n", jarExtractor.getError());
        return;
    }

    auto classFiles = preprocess::extract::ClassFileExtractor::extractAllClassFiles(*jarFile, g_appData.jarEntries);
    g_appData.classMetadatas = preprocess::parse::ClassFileParser::parseClassFiles(classFiles);

    if (g_appData.classTree) {
        for (usize i = 0; i < g_appData.classMetadatas.getSize(); ++i) {
            const auto& md = g_appData.classMetadatas[i];
            if (md.thisClassInternalName) {
                g_appData.classTree->add(md.thisClassInternalName);
            }
        }

        Fl_Tree_Item* rootItem = g_appData.classTree->root();
        if (rootItem) {
            for (int i = 0; i < rootItem->children(); ++i) {
                closeAllTreeItems(rootItem->child(i));
            }
        }

        g_appData.classTree->apply_dark_scrollbars();
        g_appData.classTree->redraw();
    }
}

void onFileOpen(Fl_Widget* widget, void* data) {
    Fl_Native_File_Chooser chooser;
    chooser.title("选择 JAR 文件");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("*.jar");

    if (chooser.show() == 0) {
        const char* path = chooser.filename();
        if (path) reloadJar(path);
    }
}

// ─── 文件保存/导出回调（异步） ──────────────────────────────

struct PackResult {
    bool success;
    const char* error;
};

static void onPackDone(void* data) {
    if (!data) return;
    auto* result = static_cast<PackResult*>(data);

    if (g_appData.statusLabel) {
        if (result->success) {
            g_appData.statusLabel->label("导出成功！");
        } else {
            char buf[256];
            snprintf(buf, sizeof(buf), "失败：%s", result->error);
            g_appData.statusLabel->label(buf);
        }
        g_appData.statusLabel->redraw();
    }

    // 3 秒后自动恢复为“就绪”
    Fl::add_timeout(3.0, resetStatusLabel);

    g_appData.isPacking = false;
    delete result;
}

void onSaveJar(Fl_Widget* widget, void* data) {
    if (g_appData.fullJarPath.empty() || g_appData.classMetadatas.getSize() == 0) {
        fl_alert("请先加载一个 JAR 文件。");
        return;
    }

    if (g_appData.isPacking.exchange(true)) {
        fl_alert("正在打包，请稍候...");
        return;
    }

    int choice = fl_choice("将覆盖原文件\n%s\n是否继续？", "取消", "确定", nullptr,
                           g_appData.fullJarPath.c_str());
    if (choice != 1) {
        g_appData.isPacking = false;
        return;
    }

    std::string inputPath = g_appData.fullJarPath;
    auto& metadatas = g_appData.classMetadatas;

    if (g_appData.statusLabel) {
        g_appData.statusLabel->label("正在保存...");
        g_appData.statusLabel->redraw();
    }

    std::thread([inputPath, &metadatas]() {
        postprocess::package::JarPackager packager;
        bool success = packager.packageJar(
            inputPath.c_str(),
            inputPath.c_str(),
            metadatas
        );
        auto* result = new PackResult{success, packager.getError()};
        Fl::awake(onPackDone, result);
    }).detach();
}

void onExportJar(Fl_Widget* widget, void* data) {
    if (g_appData.fullJarPath.empty() || g_appData.classMetadatas.getSize() == 0) {
        fl_alert("请先加载一个 JAR 文件。");
        return;
    }

    if (g_appData.isPacking.exchange(true)) {
        fl_alert("正在打包，请稍候...");
        return;
    }

    Fl_Native_File_Chooser chooser;
    chooser.title("导出 JAR 文件");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.filter("*.jar");

    std::string defaultName = g_appData.fullJarPath;
    usize dotPos = defaultName.find_last_of('.');
    if (dotPos != std::string::npos) {
        defaultName.insert(dotPos, "_modified");
    } else {
        defaultName += "_modified";
    }
    chooser.preset_file(defaultName.c_str());

    if (chooser.show() != 0) {
        g_appData.isPacking = false;
        return;
    }

    const char* outPath = chooser.filename();
    if (!outPath) {
        g_appData.isPacking = false;
        return;
    }

    std::string inputPath = g_appData.fullJarPath;
    std::string outputPath = outPath;
    auto& metadatas = g_appData.classMetadatas;

    if (g_appData.statusLabel) {
        g_appData.statusLabel->label("正在导出...");
        g_appData.statusLabel->redraw();
    }

    std::thread([inputPath, outputPath, &metadatas]() {
        postprocess::package::JarPackager packager;
        bool success = packager.packageJar(
            inputPath.c_str(),
            outputPath.c_str(),
            metadatas
        );
        auto* result = new PackResult{success, packager.getError()};
        Fl::awake(onPackDone, result);
    }).detach();
}

// ─── 主窗口 ────────────────────────────────────────────────────

class MainWindow : public Fl_Window {
public:
    MainWindow(int w, int h, const char* title) : Fl_Window(w, h, title) {}

    Fl_Menu_Bar* menuBar = nullptr;
    Fl_Box* separator = nullptr;
    Fl_Group* controlBar = nullptr;
    Fl_Group* rightArea = nullptr;
    ClickableTextDisplay* structDisplay = nullptr;
    ClickableTextDisplay* insnDisplay = nullptr;
    Fl_Box* divider = nullptr;

    int current_insn_h = 300;
    const int MARGIN_X = 8;
    const int MARGIN_Y = 8;

    int handle(int event) override {
        if (event == FL_MOUSEWHEEL) {
            int mx = Fl::event_x();
            int my = Fl::event_y();

            if (g_appData.classTree && mx < g_appData.classTree->x() + g_appData.classTree->w()) {
                g_appData.classTree->handle(event);
                return 1;
            }

            if (structDisplay && my < structDisplay->y() + structDisplay->h()) {
                structDisplay->handle(event);
                return 1;
            }

            if (insnDisplay && my >= insnDisplay->y()) {
                insnDisplay->handle(event);
                return 1;
            }
        } else if (event == FL_PUSH) {
            int mx = Fl::event_x();
            int my = Fl::event_y();

            if (divider) {
                int dy = divider->y();
                if (my >= dy - 3 && my <= dy + divider->h() + 3 &&
                    mx >= divider->x() && mx <= divider->x() + divider->w()) {
                    dragging_ = true;
                    start_y_ = my;
                    start_insn_h_ = insnDisplay->h();
                    return 1;
                }
            }
        } else if (event == FL_DRAG && dragging_) {
            int current_y = Fl::event_y();
            int delta = current_y - start_y_;
            int new_insn_h = start_insn_h_ - delta;

            if (new_insn_h < min_insn_h_) new_insn_h = min_insn_h_;
            if (new_insn_h > max_insn_h_) new_insn_h = max_insn_h_;

            apply_layout(new_insn_h);
            return 1;
        } else if (event == FL_RELEASE && dragging_) {
            dragging_ = false;
            return 1;
        } else if (event == FL_MOVE || event == FL_ENTER) {
            int mx = Fl::event_x();
            int my = Fl::event_y();

            if (divider) {
                int dy = divider->y();
                if (my >= dy - 3 && my <= dy + divider->h() + 3 &&
                    mx >= divider->x() && mx <= divider->x() + divider->w()) {
                    cursor(FL_CURSOR_NS);
                } else {
                    cursor(FL_CURSOR_DEFAULT);
                }
            }
            return Fl_Window::handle(event);
        }

        return Fl_Window::handle(event);
    }

    void apply_layout(int insn_h) {
        current_insn_h = insn_h;
        if (!structDisplay || !insnDisplay || !divider) return;

        int leftW = 280;
        int right_x = leftW;
        int right_y = controlBar->y() + controlBar->h();
        int right_w = w() - leftW;
        int right_h = h() - right_y;

        int top_h = right_h - current_insn_h - DIVIDER_H;
        if (top_h < 0) top_h = 0;

        structDisplay->resize(right_x + MARGIN_X, right_y + MARGIN_Y,
                              right_w - MARGIN_X * 2, top_h - MARGIN_Y * 2);

        divider->resize(right_x + MARGIN_X, right_y + top_h,
                        right_w - MARGIN_X * 2, DIVIDER_H);

        insnDisplay->resize(right_x + MARGIN_X, right_y + top_h + DIVIDER_H + MARGIN_Y,
                           right_w - MARGIN_X * 2, current_insn_h - MARGIN_Y * 2);

        if (rightArea) rightArea->redraw();
    }

    void resize(int x, int y, int w, int h) override {
        Fl_Window::resize(x, y, w, h);

        constexpr int menuBarH = 25, separatorH = 1, controlBarH = 30, leftW = 280;
        constexpr int baseY = menuBarH + separatorH;

        if (menuBar) menuBar->resize(0, 0, w, menuBarH);
        if (separator) separator->resize(0, menuBarH, w, separatorH);
        if (controlBar) controlBar->resize(280, baseY, w - 280, controlBarH);

        if (g_appData.fileLabel) {
            int labelW = 200;
            int labelH = menuBarH - 6;
            g_appData.fileLabel->resize((w - labelW) / 2, 3, labelW, labelH);
        }

        if (g_appData.classTree) {
            g_appData.classTree->resize(8, baseY + 8, leftW - 16, h - baseY - 16);
        }

        if (rightArea) {
            rightArea->resize(leftW, baseY + controlBarH, w - leftW, h - baseY - controlBarH);
        }

        if (structDisplay && insnDisplay && divider) {
            int right_y = controlBar->y() + controlBar->h();
            int right_w = w - leftW;
            int right_h = h - right_y;
            int top_h = right_h - current_insn_h - DIVIDER_H;
            if (top_h < 0) top_h = 0;

            structDisplay->resize(leftW + MARGIN_X, right_y + MARGIN_Y,
                                  right_w - MARGIN_X * 2, top_h - MARGIN_Y * 2);

            divider->resize(leftW + MARGIN_X, right_y + top_h,
                            right_w - MARGIN_X * 2, DIVIDER_H);

            insnDisplay->resize(leftW + MARGIN_X, right_y + top_h + DIVIDER_H + MARGIN_Y,
                                right_w - MARGIN_X * 2, current_insn_h - MARGIN_Y * 2);

            if (rightArea) rightArea->redraw();
        }
    }

private:
    static constexpr int DIVIDER_H = 3;
    bool dragging_ = false;
    int start_y_ = 0;
    int start_insn_h_ = 0;
    const int min_insn_h_ = 250;
    const int max_insn_h_ = 600;
};

// ─── 主函数 ────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    Fl_Tooltip::delay(0.2f);

    constexpr int menuBarH = 25, separatorH = 1, controlBarH = 30, leftW = 280;
    constexpr int baseY = menuBarH + separatorH;

    auto window = new MainWindow(1100, 700, "JByte Dominator");
    window->color(fl_rgb_color(43, 43, 43));

    auto menuBar = new Fl_Menu_Bar(0, 0, window->w(), menuBarH);
    menuBar->box(FL_FLAT_BOX);
    menuBar->color(fl_rgb_color(43, 43, 43));
    menuBar->textcolor(fl_rgb_color(200, 200, 200));
    menuBar->add("文件/打开", FL_CTRL + 'o', onFileOpen);
    menuBar->add("文件/保存", FL_CTRL + 's', onSaveJar);
    menuBar->add("文件/导出为...", 0, onExportJar);

    auto fileLabel = new HoverLabel((window->w() - 200) / 2, 3, 200, menuBarH - 6, "未加载文件");

    auto separator = new Fl_Box(0, menuBarH, window->w(), separatorH);
    separator->box(FL_FLAT_BOX);
    separator->color(fl_rgb_color(55, 55, 55));

    auto controlBar = new Fl_Group(280, baseY, window->w() - 280, controlBarH);
    controlBar->box(FL_FLAT_BOX);
    controlBar->color(fl_rgb_color(45, 45, 45));

    auto cpCheck = new Fl_Check_Button(290, baseY + 5, 90, 20, "常量池");
    cpCheck->box(FL_FLAT_BOX);
    cpCheck->color(fl_rgb_color(45, 45, 45));
    cpCheck->labelcolor(fl_rgb_color(200, 200, 200));
    cpCheck->value(0);
    cpCheck->callback(onToggleView);
    cpCheck->selection_color(fl_rgb_color(80, 100, 120));

    auto fieldCheck = new Fl_Check_Button(390, baseY + 5, 80, 20, "字段");
    fieldCheck->box(FL_FLAT_BOX);
    fieldCheck->color(fl_rgb_color(45, 45, 45));
    fieldCheck->labelcolor(fl_rgb_color(200, 200, 200));
    fieldCheck->value(1);
    fieldCheck->callback(onToggleView);
    fieldCheck->selection_color(fl_rgb_color(80, 100, 120));

    auto methodCheck = new Fl_Check_Button(480, baseY + 5, 80, 20, "方法");
    methodCheck->box(FL_FLAT_BOX);
    methodCheck->color(fl_rgb_color(45, 45, 45));
    methodCheck->labelcolor(fl_rgb_color(200, 200, 200));
    methodCheck->value(1);
    methodCheck->callback(onToggleView);
    methodCheck->selection_color(fl_rgb_color(80, 100, 120));

    // ─── 状态标签 ──────────────────────────────────────────
    auto statusLabel = new Fl_Box(580, baseY + 5, 220, 20, "就绪");
    statusLabel->box(FL_FLAT_BOX);
    statusLabel->color(fl_rgb_color(45, 45, 45));
    statusLabel->labelcolor(fl_rgb_color(180, 220, 180));
    statusLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    statusLabel->labelsize(12);
    g_appData.statusLabel = statusLabel;

    controlBar->end();

    auto classTree = new CustomTree(8, baseY + 8, leftW - 16, window->h() - baseY - 16);
    classTree->box(FL_FLAT_BOX);
    classTree->color(fl_rgb_color(45, 45, 45));
    classTree->selectmode(FL_TREE_SELECT_SINGLE);
    classTree->callback(onClassSelected);
    classTree->showroot(0);
    classTree->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    classTree->item_reselect_mode(FL_TREE_SELECTABLE_ALWAYS);
    classTree->item_labelfgcolor(fl_rgb_color(187, 187, 187));
    classTree->item_labelbgcolor(fl_rgb_color(45, 45, 45));
    classTree->item_labelsize(12);
    classTree->selection_color(fl_rgb_color(75, 95, 115));

    int right_h = window->h() - baseY - controlBarH;
    int init_bottom_h = 300;
    int init_top_h = right_h - init_bottom_h - 3;

    auto rightArea = new Fl_Group(leftW, baseY + controlBarH, window->w() - leftW, right_h);
    rightArea->box(FL_FLAT_BOX);
    rightArea->color(fl_rgb_color(43, 43, 43));

    auto structDisplay = new ClickableTextDisplay(
        leftW + window->MARGIN_X,
        baseY + controlBarH + window->MARGIN_Y,
        window->w() - leftW - window->MARGIN_X * 2,
        init_top_h - window->MARGIN_Y * 2
    );
    structDisplay->box(FL_FLAT_BOX);
    structDisplay->color(fl_rgb_color(37, 37, 38));
    structDisplay->textcolor(fl_rgb_color(187, 187, 187));
    structDisplay->cursor_color(fl_rgb_color(200, 200, 200));
    structDisplay->selection_color(fl_rgb_color(60, 80, 100));
    structDisplay->textfont(FL_COURIER);
    structDisplay->textsize(13);
    structDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

    auto structBuffer = new Fl_Text_Buffer();
    structDisplay->buffer(structBuffer);
    structDisplay->handle_click_callback([&]() { onStructClicked(structDisplay, nullptr); });

    auto divider = new Fl_Box(
        leftW + window->MARGIN_X,
        baseY + controlBarH + init_top_h,
        window->w() - leftW - window->MARGIN_X * 2,
        3
    );
    divider->box(FL_FLAT_BOX);
    divider->color(fl_rgb_color(55, 55, 55));

    auto insnDisplay = new ClickableTextDisplay(
        leftW + window->MARGIN_X,
        baseY + controlBarH + init_top_h + 3 + window->MARGIN_Y,
        window->w() - leftW - window->MARGIN_X * 2,
        init_bottom_h - window->MARGIN_Y * 2
    );
    insnDisplay->box(FL_FLAT_BOX);
    insnDisplay->color(fl_rgb_color(30, 30, 30));
    insnDisplay->textcolor(fl_rgb_color(187, 187, 187));
    insnDisplay->cursor_color(fl_rgb_color(200, 200, 200));
    insnDisplay->selection_color(fl_rgb_color(60, 80, 100));
    insnDisplay->textfont(FL_COURIER);
    insnDisplay->textsize(13);
    insnDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

    auto insnBuffer = new Fl_Text_Buffer();
    insnDisplay->buffer(insnBuffer);

    // ─── 绑定指令流右键菜单 ──────────────────────────────────

    // 指令编码
    enum {
        NOP,
        PRINTLN,
        ACONST_NULL,
        ICONST_M1,
        ICONST_0, ICONST_1, ICONST_2, ICONST_3, ICONST_4, ICONST_5,
        BIPUSH,
        SIPUSH,
        LDC,
        ILOAD_0, ILOAD_1, ILOAD_2, ILOAD_3,
        LLOAD_0, LLOAD_1, LLOAD_2, LLOAD_3,
        FLOAD_0, FLOAD_1, FLOAD_2, FLOAD_3,
        DLOAD_0, DLOAD_1, DLOAD_2, DLOAD_3,
        ALOAD_0, ALOAD_1, ALOAD_2, ALOAD_3,
        ISTORE_0, ISTORE_1, ISTORE_2, ISTORE_3,
        LSTORE_0, LSTORE_1, LSTORE_2, LSTORE_3,
        FSTORE_0, FSTORE_1, FSTORE_2, FSTORE_3,
        DSTORE_0, DSTORE_1, DSTORE_2, DSTORE_3,
        ASTORE_0, ASTORE_1, ASTORE_2, ASTORE_3,
        IADD, ISUB, IMUL, IDIV, IREM, INEG,
        ISHL, ISHR, IUSHR,
        IAND, IOR, IXOR,
        LCMP, FCMPL, FCMPG, DCMPL, DCMPG,
        NEW,
        ARRAYLENGTH,
        CHECKCAST,
        INSTANCEOF,
        GETSTATIC, PUTSTATIC,
        GETFIELD, PUTFIELD,
        INVOKESPECIAL, INVOKESTATIC, INVOKEINTERFACE,
        IRETURN, RETURN,
        DELETE_ = 0x7FFF
    };

    // 菜单生成宏
    #define MENU_BEFORE(code) { #code, 0, nullptr, (void*)(intptr_t)(0 << 16 | (code)), 0 }
    #define MENU_AFTER(code)  { #code, 0, nullptr, (void*)(intptr_t)(1 << 16 | (code)), 0 }

    // 将菜单数组静态化，避免栈溢出
    static Fl_Menu_Item menu[] = {
        // ---------- 在前插入 ----------
        {"在前插入", 0, nullptr, nullptr, FL_SUBMENU},
            {"基础指令", 0, nullptr, nullptr, FL_SUBMENU},
                {"常量加载", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(ACONST_NULL),
                    MENU_BEFORE(ICONST_M1),
                    MENU_BEFORE(ICONST_0),
                    MENU_BEFORE(ICONST_1),
                    MENU_BEFORE(ICONST_2),
                    MENU_BEFORE(ICONST_3),
                    MENU_BEFORE(ICONST_4),
                    MENU_BEFORE(ICONST_5),
                    MENU_BEFORE(BIPUSH),
                    MENU_BEFORE(SIPUSH),
                    MENU_BEFORE(LDC),
                    {nullptr},
                {"加载", 0, nullptr, nullptr, FL_SUBMENU},
                    {"int", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(ILOAD_0),
                        MENU_BEFORE(ILOAD_1),
                        MENU_BEFORE(ILOAD_2),
                        MENU_BEFORE(ILOAD_3),
                        {nullptr},
                    {"long", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(LLOAD_0),
                        MENU_BEFORE(LLOAD_1),
                        MENU_BEFORE(LLOAD_2),
                        MENU_BEFORE(LLOAD_3),
                        {nullptr},
                    {"float", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(FLOAD_0),
                        MENU_BEFORE(FLOAD_1),
                        MENU_BEFORE(FLOAD_2),
                        MENU_BEFORE(FLOAD_3),
                        {nullptr},
                    {"double", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(DLOAD_0),
                        MENU_BEFORE(DLOAD_1),
                        MENU_BEFORE(DLOAD_2),
                        MENU_BEFORE(DLOAD_3),
                        {nullptr},
                    {"reference", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(ALOAD_0),
                        MENU_BEFORE(ALOAD_1),
                        MENU_BEFORE(ALOAD_2),
                        MENU_BEFORE(ALOAD_3),
                        {nullptr},
                    {nullptr},
                {"存储", 0, nullptr, nullptr, FL_SUBMENU},
                    {"int", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(ISTORE_0),
                        MENU_BEFORE(ISTORE_1),
                        MENU_BEFORE(ISTORE_2),
                        MENU_BEFORE(ISTORE_3),
                        {nullptr},
                    {"long", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(LSTORE_0),
                        MENU_BEFORE(LSTORE_1),
                        MENU_BEFORE(LSTORE_2),
                        MENU_BEFORE(LSTORE_3),
                        {nullptr},
                    {"float", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(FSTORE_0),
                        MENU_BEFORE(FSTORE_1),
                        MENU_BEFORE(FSTORE_2),
                        MENU_BEFORE(FSTORE_3),
                        {nullptr},
                    {"double", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(DSTORE_0),
                        MENU_BEFORE(DSTORE_1),
                        MENU_BEFORE(DSTORE_2),
                        MENU_BEFORE(DSTORE_3),
                        {nullptr},
                    {"reference", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_BEFORE(ASTORE_0),
                        MENU_BEFORE(ASTORE_1),
                        MENU_BEFORE(ASTORE_2),
                        MENU_BEFORE(ASTORE_3),
                        {nullptr},
                    {nullptr},
                {"算术", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(IADD),
                    MENU_BEFORE(ISUB),
                    MENU_BEFORE(IMUL),
                    MENU_BEFORE(IDIV),
                    MENU_BEFORE(IREM),
                    MENU_BEFORE(INEG),
                    {nullptr},
                {"移位", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(ISHL),
                    MENU_BEFORE(ISHR),
                    MENU_BEFORE(IUSHR),
                    {nullptr},
                {"位运算", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(IAND),
                    MENU_BEFORE(IOR),
                    MENU_BEFORE(IXOR),
                    {nullptr},
                {"比较", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(LCMP),
                    MENU_BEFORE(FCMPL),
                    MENU_BEFORE(FCMPG),
                    MENU_BEFORE(DCMPL),
                    MENU_BEFORE(DCMPG),
                    {nullptr},
                {"返回", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_BEFORE(IRETURN),
                    MENU_BEFORE(RETURN),
                    {nullptr},
                {nullptr},
            {"对象", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_BEFORE(NEW),
                MENU_BEFORE(ARRAYLENGTH),
                MENU_BEFORE(CHECKCAST),
                MENU_BEFORE(INSTANCEOF),
                {nullptr},
            {"字段", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_BEFORE(GETSTATIC),
                MENU_BEFORE(PUTSTATIC),
                MENU_BEFORE(GETFIELD),
                MENU_BEFORE(PUTFIELD),
                {nullptr},
            {"调用", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_BEFORE(INVOKESPECIAL),
                MENU_BEFORE(INVOKESTATIC),
                MENU_BEFORE(INVOKEINTERFACE),
                {nullptr},
            {"复合指令", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_BEFORE(PRINTLN),
                {nullptr},
            MENU_BEFORE(NOP),
            {nullptr},

        // ---------- 在后插入 ----------
        {"在后插入", 0, nullptr, nullptr, FL_SUBMENU},
            {"基础指令", 0, nullptr, nullptr, FL_SUBMENU},
                {"常量加载", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(ACONST_NULL),
                    MENU_AFTER(ICONST_M1),
                    MENU_AFTER(ICONST_0),
                    MENU_AFTER(ICONST_1),
                    MENU_AFTER(ICONST_2),
                    MENU_AFTER(ICONST_3),
                    MENU_AFTER(ICONST_4),
                    MENU_AFTER(ICONST_5),
                    MENU_AFTER(BIPUSH),
                    MENU_AFTER(SIPUSH),
                    MENU_AFTER(LDC),
                    {nullptr},
                {"加载", 0, nullptr, nullptr, FL_SUBMENU},
                    {"int", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(ILOAD_0),
                        MENU_AFTER(ILOAD_1),
                        MENU_AFTER(ILOAD_2),
                        MENU_AFTER(ILOAD_3),
                        {nullptr},
                    {"long", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(LLOAD_0),
                        MENU_AFTER(LLOAD_1),
                        MENU_AFTER(LLOAD_2),
                        MENU_AFTER(LLOAD_3),
                        {nullptr},
                    {"float", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(FLOAD_0),
                        MENU_AFTER(FLOAD_1),
                        MENU_AFTER(FLOAD_2),
                        MENU_AFTER(FLOAD_3),
                        {nullptr},
                    {"double", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(DLOAD_0),
                        MENU_AFTER(DLOAD_1),
                        MENU_AFTER(DLOAD_2),
                        MENU_AFTER(DLOAD_3),
                        {nullptr},
                    {"reference", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(ALOAD_0),
                        MENU_AFTER(ALOAD_1),
                        MENU_AFTER(ALOAD_2),
                        MENU_AFTER(ALOAD_3),
                        {nullptr},
                    {nullptr},
                {"存储", 0, nullptr, nullptr, FL_SUBMENU},
                    {"int", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(ISTORE_0),
                        MENU_AFTER(ISTORE_1),
                        MENU_AFTER(ISTORE_2),
                        MENU_AFTER(ISTORE_3),
                        {nullptr},
                    {"long", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(LSTORE_0),
                        MENU_AFTER(LSTORE_1),
                        MENU_AFTER(LSTORE_2),
                        MENU_AFTER(LSTORE_3),
                        {nullptr},
                    {"float", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(FSTORE_0),
                        MENU_AFTER(FSTORE_1),
                        MENU_AFTER(FSTORE_2),
                        MENU_AFTER(FSTORE_3),
                        {nullptr},
                    {"double", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(DSTORE_0),
                        MENU_AFTER(DSTORE_1),
                        MENU_AFTER(DSTORE_2),
                        MENU_AFTER(DSTORE_3),
                        {nullptr},
                    {"reference", 0, nullptr, nullptr, FL_SUBMENU},
                        MENU_AFTER(ASTORE_0),
                        MENU_AFTER(ASTORE_1),
                        MENU_AFTER(ASTORE_2),
                        MENU_AFTER(ASTORE_3),
                        {nullptr},
                    {nullptr},
                {"算术", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(IADD),
                    MENU_AFTER(ISUB),
                    MENU_AFTER(IMUL),
                    MENU_AFTER(IDIV),
                    MENU_AFTER(IREM),
                    MENU_AFTER(INEG),
                    {nullptr},
                {"移位", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(ISHL),
                    MENU_AFTER(ISHR),
                    MENU_AFTER(IUSHR),
                    {nullptr},
                {"位运算", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(IAND),
                    MENU_AFTER(IOR),
                    MENU_AFTER(IXOR),
                    {nullptr},
                {"比较", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(LCMP),
                    MENU_AFTER(FCMPL),
                    MENU_AFTER(FCMPG),
                    MENU_AFTER(DCMPL),
                    MENU_AFTER(DCMPG),
                    {nullptr},
                {"返回", 0, nullptr, nullptr, FL_SUBMENU},
                    MENU_AFTER(IRETURN),
                    MENU_AFTER(RETURN),
                    {nullptr},
                {nullptr},
            {"对象", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_AFTER(NEW),
                MENU_AFTER(ARRAYLENGTH),
                MENU_AFTER(CHECKCAST),
                MENU_AFTER(INSTANCEOF),
                {nullptr},
            {"字段", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_AFTER(GETSTATIC),
                MENU_AFTER(PUTSTATIC),
                MENU_AFTER(GETFIELD),
                MENU_AFTER(PUTFIELD),
                {nullptr},
            {"调用", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_AFTER(INVOKESPECIAL),
                MENU_AFTER(INVOKESTATIC),
                MENU_AFTER(INVOKEINTERFACE),
                {nullptr},
            {"复合指令", 0, nullptr, nullptr, FL_SUBMENU},
                MENU_AFTER(PRINTLN),
                {nullptr},
            MENU_AFTER(NOP),
            {nullptr},

        // ---------- 删除 ----------
        {"删除", 0, nullptr, (void*)(intptr_t)(2 << 16 | DELETE_), 0},
        {nullptr}
    };

    #undef MENU_BEFORE
    #undef MENU_AFTER

    insnDisplay->handle_right_click_callback([&](int lineStart) {
        // 检查是否正在打包
        if (g_appData.isPacking) {
            fl_alert("正在打包，请稍候再修改指令。");
            return;
        }

        auto it = g_appData.insnLineToIndex.find(lineStart);
        if (it == g_appData.insnLineToIndex.end()) {
            fl_alert("无法定位该指令行。");
            return;
        }
        g_appData.currentInsnIndex = it->second;

        if (g_appData.currentClassIndex < 0 || g_appData.currentClassIndex >= (int)g_appData.classMetadatas.getSize()) {
            fl_alert("当前类索引无效。");
            return;
        }
        if (!g_appData.currentMetadata) {
            fl_alert("当前类数据无效。");
            return;
        }

        auto& m = g_appData.classMetadatas[g_appData.currentClassIndex];
        if (g_appData.selectedMethodIndex >= m.methods.getSize()) {
            fl_alert("当前方法索引无效。");
            return;
        }

        const Fl_Menu_Item* selected = menu->popup(Fl::event_x(), Fl::event_y());
        if (!selected) return;

        int action = (int)(intptr_t)selected->user_data();
        int position = action >> 16;
        int code = action & 0xFFFF;

        auto& method = m.methods[g_appData.selectedMethodIndex];

        if (position == 2) {
            runtime::modify::InstructionsModifier modifier(m, method);
            if (modifier.removeAt(g_appData.currentInsnIndex) && modifier.applyAllModification()) {
                updateStructDisplay();
                std::string insnText = renderInstructions(m, method.bytecodes);
                g_appData.insnBuffer->text(insnText.c_str());
                g_appData.insnDisplay->redraw();
            } else {
                fl_alert("删除失败");
            }
            return;
        }

        bool before = (position == 0);
        runtime::modify::InstructionsModifier modifier(m, method);
        model::bytecode::Instruction insn;

        switch (code) {
            case NOP:
                insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::nop);
                break;
            case PRINTLN:
                {
                    const char* str = fl_input("请输入要打印的字符串：", "Hello");
                    if (!str) return;
                    bool ok = before ?
                        modifier.insertBefore(g_appData.currentInsnIndex, {
                            modifier.makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
                            modifier.makeLdcString(str),
                            modifier.makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
                        }) :
                        modifier.insertAfter(g_appData.currentInsnIndex, {
                            modifier.makeGetStatic("java/lang/System", "out", "Ljava/io/PrintStream;"),
                            modifier.makeLdcString(str),
                            modifier.makeInvokeVirtual("java/io/PrintStream", "println", "(Ljava/lang/String;)V")
                        });
                    if (ok && modifier.applyAllModification()) {
                        updateStructDisplay();
                        std::string insnText = renderInstructions(m, method.bytecodes);
                        g_appData.insnBuffer->text(insnText.c_str());
                        g_appData.insnDisplay->redraw();
                    } else {
                        fl_alert("插入失败");
                    }
                    return;
                }
            case ACONST_NULL: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::aconst_null); break;
            case ICONST_M1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_m1); break;
            case ICONST_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_0); break;
            case ICONST_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_1); break;
            case ICONST_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_2); break;
            case ICONST_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_3); break;
            case ICONST_4: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_4); break;
            case ICONST_5: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iconst_5); break;
            case BIPUSH:
                {
                    const char* valStr = fl_input("请输入 bipush 数值 (byte):", "10");
                    if (!valStr) return;
                    int val = atoi(valStr);
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::bipush, (u8)val);
                }
                break;
            case SIPUSH:
                {
                    const char* valStr = fl_input("请输入 sipush 数值 (short):", "100");
                    if (!valStr) return;
                    int val = atoi(valStr);
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::sipush, (u16)val);
                }
                break;
            case LDC:
                {
                    const char* str = fl_input("请输入字符串常量:", "Hello");
                    if (!str) return;
                    insn = modifier.makeLdcString(str);
                }
                break;
            case ILOAD_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iload_0); break;
            case ILOAD_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iload_1); break;
            case ILOAD_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iload_2); break;
            case ILOAD_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iload_3); break;
            case LLOAD_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lload_0); break;
            case LLOAD_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lload_1); break;
            case LLOAD_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lload_2); break;
            case LLOAD_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lload_3); break;
            case FLOAD_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fload_0); break;
            case FLOAD_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fload_1); break;
            case FLOAD_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fload_2); break;
            case FLOAD_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fload_3); break;
            case DLOAD_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dload_0); break;
            case DLOAD_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dload_1); break;
            case DLOAD_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dload_2); break;
            case DLOAD_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dload_3); break;
            case ALOAD_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::aload_0); break;
            case ALOAD_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::aload_1); break;
            case ALOAD_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::aload_2); break;
            case ALOAD_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::aload_3); break;
            case ISTORE_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::istore_0); break;
            case ISTORE_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::istore_1); break;
            case ISTORE_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::istore_2); break;
            case ISTORE_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::istore_3); break;
            case LSTORE_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lstore_0); break;
            case LSTORE_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lstore_1); break;
            case LSTORE_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lstore_2); break;
            case LSTORE_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lstore_3); break;
            case FSTORE_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fstore_0); break;
            case FSTORE_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fstore_1); break;
            case FSTORE_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fstore_2); break;
            case FSTORE_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fstore_3); break;
            case DSTORE_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dstore_0); break;
            case DSTORE_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dstore_1); break;
            case DSTORE_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dstore_2); break;
            case DSTORE_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dstore_3); break;
            case ASTORE_0: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::astore_0); break;
            case ASTORE_1: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::astore_1); break;
            case ASTORE_2: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::astore_2); break;
            case ASTORE_3: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::astore_3); break;
            case IADD: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iadd); break;
            case ISUB: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::isub); break;
            case IMUL: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::imul); break;
            case IDIV: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::idiv); break;
            case IREM: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::irem); break;
            case INEG: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ineg); break;
            case ISHL: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ishl); break;
            case ISHR: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ishr); break;
            case IUSHR: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iushr); break;
            case IAND: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::iand); break;
            case IOR: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ior); break;
            case IXOR: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ixor); break;
            case LCMP: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::lcmp); break;
            case FCMPL: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fcmpl); break;
            case FCMPG: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::fcmpg); break;
            case DCMPL: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dcmpl); break;
            case DCMPG: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::dcmpg); break;
            case IRETURN: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::ireturn); break;
            case RETURN: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::return_); break;
            case NEW:
                {
                    const char* cls_tmp = fl_input("请输入类名（内部形式，如 java/lang/Object）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    u16 idx = m.addClassEntry(cls);
                    free(cls);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::new_, idx);
                }
                break;
            case ARRAYLENGTH: insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::arraylength); break;
            case CHECKCAST:
                {
                    const char* cls_tmp = fl_input("请输入类名（内部形式）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    u16 idx = m.addClassEntry(cls);
                    free(cls);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::checkcast, idx);
                }
                break;
            case INSTANCEOF:
                {
                    const char* cls_tmp = fl_input("请输入类名（内部形式）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    u16 idx = m.addClassEntry(cls);
                    free(cls);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::instanceof, idx);
                }
                break;
            case GETSTATIC:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/System");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* fld_tmp = fl_input("字段名:", "out");
                    if (!fld_tmp) { free(cls); return; }
                    char* fld = strdup(fld_tmp);
                    const char* desc_tmp = fl_input("字段描述符:", "Ljava/io/PrintStream;");
                    if (!desc_tmp) { free(cls); free(fld); return; }
                    char* desc = strdup(desc_tmp);
                    insn = modifier.makeGetStatic(cls, fld, desc);
                    free(cls); free(fld); free(desc);
                }
                break;
            case PUTSTATIC:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/System");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* fld_tmp = fl_input("字段名:", "out");
                    if (!fld_tmp) { free(cls); return; }
                    char* fld = strdup(fld_tmp);
                    const char* desc_tmp = fl_input("字段描述符:", "Ljava/io/PrintStream;");
                    if (!desc_tmp) { free(cls); free(fld); return; }
                    char* desc = strdup(desc_tmp);
                    insn = modifier.makePutStatic(cls, fld, desc);
                    free(cls); free(fld); free(desc);
                }
                break;
            case GETFIELD:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* fld_tmp = fl_input("字段名:", "value");
                    if (!fld_tmp) { free(cls); return; }
                    char* fld = strdup(fld_tmp);
                    const char* desc_tmp = fl_input("字段描述符:", "Ljava/lang/Object;");
                    if (!desc_tmp) { free(cls); free(fld); return; }
                    char* desc = strdup(desc_tmp);
                    u16 idx = m.addFieldrefEntry(cls, fld, desc);
                    free(cls); free(fld); free(desc);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::getfield, idx);
                }
                break;
            case PUTFIELD:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* fld_tmp = fl_input("字段名:", "value");
                    if (!fld_tmp) { free(cls); return; }
                    char* fld = strdup(fld_tmp);
                    const char* desc_tmp = fl_input("字段描述符:", "Ljava/lang/Object;");
                    if (!desc_tmp) { free(cls); free(fld); return; }
                    char* desc = strdup(desc_tmp);
                    u16 idx = m.addFieldrefEntry(cls, fld, desc);
                    free(cls); free(fld); free(desc);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::putfield, idx);
                }
                break;
            case INVOKESPECIAL:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/Object");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* name_tmp = fl_input("方法名:", "<init>");
                    if (!name_tmp) { free(cls); return; }
                    char* name = strdup(name_tmp);
                    const char* desc_tmp = fl_input("方法描述符:", "()V");
                    if (!desc_tmp) { free(cls); free(name); return; }
                    char* desc = strdup(desc_tmp);
                    insn = modifier.makeInvokeSpecial(cls, name, desc);
                    free(cls); free(name); free(desc);
                }
                break;
            case INVOKESTATIC:
                {
                    const char* cls_tmp = fl_input("类名（内部形式）:", "java/lang/System");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* name_tmp = fl_input("方法名:", "currentTimeMillis");
                    if (!name_tmp) { free(cls); return; }
                    char* name = strdup(name_tmp);
                    const char* desc_tmp = fl_input("方法描述符:", "()J");
                    if (!desc_tmp) { free(cls); free(name); return; }
                    char* desc = strdup(desc_tmp);
                    insn = modifier.makeInvokeStatic(cls, name, desc);
                    free(cls); free(name); free(desc);
                }
                break;
            case INVOKEINTERFACE:
                {
                    const char* cls_tmp = fl_input("接口名（内部形式）:", "java/lang/Runnable");
                    if (!cls_tmp) return;
                    char* cls = strdup(cls_tmp);
                    const char* name_tmp = fl_input("方法名:", "run");
                    if (!name_tmp) { free(cls); return; }
                    char* name = strdup(name_tmp);
                    const char* desc_tmp = fl_input("方法描述符:", "()V");
                    if (!desc_tmp) { free(cls); free(name); return; }
                    char* desc = strdup(desc_tmp);
                    u16 idx = m.addMethodrefEntry(cls, name, desc);
                    free(cls); free(name); free(desc);
                    if (idx == 0) { fl_alert("添加常量池失败"); return; }
                    insn = model::bytecode::Instruction::make(model::bytecode::Opcodes::invokeinterface, idx);
                }
                break;
            default:
                fl_alert("未知指令");
                return;
        }

        bool ok = before ?
            modifier.insertBefore(g_appData.currentInsnIndex, insn) :
            modifier.insertAfter(g_appData.currentInsnIndex, insn);
        if (ok && modifier.applyAllModification()) {
            updateStructDisplay();
            std::string insnText = renderInstructions(m, method.bytecodes);
            g_appData.insnBuffer->text(insnText.c_str());
            g_appData.insnDisplay->redraw();
        } else {
            fl_alert("插入失败");
        }
    });

    rightArea->add(structDisplay);
    rightArea->add(divider);
    rightArea->add(insnDisplay);
    rightArea->end();

    window->add(classTree);
    window->add(rightArea);

    window->menuBar = menuBar;
    window->separator = separator;
    window->controlBar = controlBar;
    window->rightArea = rightArea;
    window->structDisplay = structDisplay;
    window->insnDisplay = insnDisplay;
    window->divider = divider;
    window->current_insn_h = init_bottom_h;

    g_appData.classTree = classTree;
    g_appData.structDisplay = structDisplay;
    g_appData.structBuffer = structBuffer;
    g_appData.insnDisplay = insnDisplay;
    g_appData.insnBuffer = insnBuffer;
    g_appData.fileLabel = fileLabel;
    g_appData.cpCheck = cpCheck;
    g_appData.fieldCheck = fieldCheck;
    g_appData.methodCheck = methodCheck;

    if (argc > 1) reloadJar(argv[1]);

    window->resizable(window);
    window->end();

    int min_win_w = leftW + 400;
    int min_win_h = baseY + controlBarH + 250 + 3 + 100 + 16;
    window->size_range(min_win_w, min_win_h);

    window->show(argc, argv);
    return Fl::run();
}