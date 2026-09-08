# JByte Dominator

A lightweight C++ tool for parsing, modifying, and repackaging Java JAR files – with a focus on bytecode editing.

> **Learning project** – built to explore C/C++ internals, memory management, and binary file formats.  
> **Also a serious tool** – fully functional for analysis and modification of Java class files.

---

## 📦 Features

- **JAR extraction** – read any JAR file and list/access its entries
- **Class file parsing** – full support for magic, version, constant pool, access flags, fields, methods, and attributes (including `Code`)
- **Bytecode disassembly** – parse instruction streams into a structured representation (supports `tableswitch`/`lookupswitch`)
- **Bytecode modification** – insert or delete instructions anywhere, with automatic:
  - Offset recalculation
  - Branch offset correction
  - Exception table adjustment
- **Class serialization** – rewrite modified class data back to binary format
- **JAR repackaging** – combine modified classes with unchanged resources into a new JAR

### Two interfaces

| Interface                | Description                                                                                                                 |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| **CLI** (`jbytedom_cli`) | Command-line tool for batch processing and testing                                                                          |
| **GUI** (`jbytedom`)     | Visual browser with class tree, constant pool, fields, methods, and bytecode instruction view with right‑click modification |

---

## 🚀 Quick Start

### Build from source

```bash
git clone https://github.com/yourusername/jbytedom.git
cd jbytedom
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Dependencies** (automatically fetched via CMake):
- [zlib](https://zlib.net/)
- [libzip](https://libzip.org/)
- [FLTK](https://www.fltk.org/) – required only for GUI (pre‑compiled static library expected at `external/fltk-1.4.5/lib/fltk.lib` on Windows)

### CLI usage

```bash
./jbytedom_cli path/to/your.jar
```

This will parse the JAR, extract all class files, parse their metadata, modify the first method (insert a "Hello from JByte Dominator!" print statement), and write a modified `.class` file and a repackaged JAR in the `resource/` folder.

### GUI usage

```bash
./jbytedom
```

- **File → Open** to load a JAR
- Browse classes in the left tree
- Toggle visibility of constant pool, fields, and methods via checkboxes
- Click a method line to view its bytecode instructions in the bottom panel
- Right‑click an instruction to **insert before**, **insert after**, or **delete**
- Save changes to the original JAR or export as a new file

---

## 🧠 Design Philosophy

- **Core logic in pure C style** – using raw pointers, `malloc`/`free`, bitwise operations, and manual memory management for maximum control and minimal overhead.
- **C++ as an organizational layer** – templates (`ElasticArray`, `Span`), RAII for resource cleanup, namespaces, and function overloading are used to structure the code, but the underlying algorithms remain C‑style.
- **Explicit ownership** – functions returning heap memory are clearly suffixed with `Dup` or `Make`, making the caller responsible for `free()`.
- **No external C++ runtime dependencies** – the tool links only against zlib, libzip, and FLTK (for GUI), keeping it lightweight.

---

## ⚠️ Limitations & Known Issues

- **Instruction coverage** – only common opcodes are fully supported (load/store, arithmetic, comparisons, branches, field access, method invocation, object creation, etc.). Some instructions (`jsr`, `ret`, `athrow`, `monitorenter`, `monitorexit`, `wide` prefix handling for some cases) are partially or not yet implemented.
- **Modifier robustness** – the instruction relocator has been tested on simple methods but may not handle complex control flow (e.g., nested exception handlers) correctly in all cases.
- **Constant pool modification** – adding new constants works for strings, classes, and method/field references, but some entry types (e.g., `MethodHandle`, `Dynamic`) are not automatically managed.
- **GUI stability** – the FLTK interface is functional but lacks advanced error handling and multi‑thread safety (modifications during background tasks are blocked but not fully guarded).
- **Platform support** – primarily developed and tested on Windows (MSVC). Linux and macOS builds may require adjustments, especially for FLTK.
- **Error reporting** – parsing failures often return empty objects; detailed error messages are limited.
- **Memory usage** – entire JAR files are loaded into memory; large files (>500 MB) may cause out‑of‑memory conditions.
- **Documentation** – no extensive API docs yet; interfaces may change in future versions.

---

## 🛣️ Roadmap

- Add support for remaining bytecode instructions
- Improve control‑flow analysis for robust modification
- Command‑line mode for inserting specific instructions without GUI
- Scriptable interface (e.g., JSON input for modifications)
- Streaming JAR reader to reduce memory footprint
- Cross‑platform CI builds (Linux, macOS)

---

## 📄 License

MIT License – free to use, modify, and distribute.  
Attribution appreciated but not required.

---

> **Built as a learning journey, crafted to be useful.**  
> Happy bytecode hacking!