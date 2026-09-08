# === 配置 CMake === #
cmake_minimum_required(VERSION 3.20)


# === 配置项目 === #
project(jbytedom)


# === 配置 C++ === #
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# === 配置通用编译器选项 === #
option(RELEASE_WITH_DEBUG "Build Release with debug info (for easier debugging)" OFF)
if(MSVC)
	if(RELEASE_WITH_DEBUG)
		# 调试专用 Release：禁用所有优化，保留完整调试信息
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O0")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
	else()
		# 原有优化 Release
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy /GL /fp:fast")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
	endif()
	add_compile_options(/utf-8)
endif()


# === 配置项目文件 === #
add_executable(jbytedom
	src/Bootstrap.cpp
	test/util/Logger.hpp
	src/CommonPrimitives.hpp
	src/model/jar/JarFile.hpp
	src/model/class/ClassFile.hpp
	src/model/jar/JarFile.cpp
	src/preprocess/extract/JarExtractor.cpp
	src/preprocess/extract/JarExtractor.hpp
	src/model/jar/JarEntry.hpp
	src/CommonMacro.hpp
	src/util/FileUtils.cpp
	src/util/FileUtils.hpp
	src/util/StringUtils.cpp
	src/util/StringUtils.hpp
	src/model/common/ElasticArray.hpp
	src/model/jar/JarEntry.cpp
	src/preprocess/extract/ClassFileExtractor.hpp
	src/preprocess/extract/ClassFileExtractor.cpp
	src/model/class/ClassFile.cpp
	src/model/class/ClassMetadata.hpp
	src/preprocess/parse/ClassFileParser.hpp
	src/preprocess/parse/ClassFileParser.cpp
	src/util/ByteUtils.hpp
	src/util/ByteUtils.cpp
	src/model/class/ClassMetadata.cpp
	src/preprocess/parse/BytecodesParser.hpp
	src/model/bytecode/Opcode.hpp
	src/model/bytecode/Operand.hpp
	src/model/bytecode/Instruction.hpp
	src/model/bytecode/Instruction.cpp
	src/preprocess/parse/BytecodesParser.cpp
	src/postprocess/serialize/ClassSerializer.hpp
	src/postprocess/serialize/ClassSerializer.cpp
	src/runtime/modify/InstructionsModifier.cpp
	src/runtime/modify/InstructionsModifier.hpp
	src/runtime/compute/StackFrameComputer.cpp
	src/runtime/compute/StackFrameComputer.hpp
	src/model/bytecode/OpcodeTable.hpp
	src/postprocess/package/JarPackager.cpp
	src/postprocess/package/JarPackager.hpp
	src/util/HexUtils.hpp
	src/util/HexUtils.cpp
	src/CommonStruct.hpp
)

add_executable(jbytedom_cli
	test/Main.cpp
	test/util/Logger.hpp
	src/CommonPrimitives.hpp
	src/model/jar/JarFile.hpp
	src/model/class/ClassFile.hpp
	src/model/jar/JarFile.cpp
	src/preprocess/extract/JarExtractor.cpp
	src/preprocess/extract/JarExtractor.hpp
	src/model/jar/JarEntry.hpp
	src/CommonMacro.hpp
	src/util/FileUtils.cpp
	src/util/FileUtils.hpp
	src/util/StringUtils.cpp
	src/util/StringUtils.hpp
	src/model/common/ElasticArray.hpp
	src/model/jar/JarEntry.cpp
	src/preprocess/extract/ClassFileExtractor.hpp
	src/preprocess/extract/ClassFileExtractor.cpp
	src/model/class/ClassFile.cpp
	src/model/class/ClassMetadata.hpp
	src/preprocess/parse/ClassFileParser.hpp
	src/preprocess/parse/ClassFileParser.cpp
	src/util/ByteUtils.hpp
	src/util/ByteUtils.cpp
	src/model/class/ClassMetadata.cpp
	src/preprocess/parse/BytecodesParser.hpp
	src/model/bytecode/Opcode.hpp
	src/model/bytecode/Operand.hpp
	src/model/bytecode/Instruction.hpp
	src/model/bytecode/Instruction.cpp
	src/preprocess/parse/BytecodesParser.cpp
	src/postprocess/serialize/ClassSerializer.hpp
	src/postprocess/serialize/ClassSerializer.cpp
	src/runtime/modify/InstructionsModifier.cpp
	src/runtime/modify/InstructionsModifier.hpp
	src/runtime/compute/StackFrameComputer.cpp
	src/runtime/compute/StackFrameComputer.hpp
	src/model/bytecode/OpcodeTable.hpp
	src/postprocess/package/JarPackager.cpp
	src/postprocess/package/JarPackager.hpp
	src/util/HexUtils.hpp
	src/util/HexUtils.cpp
	src/CommonStruct.hpp
)


# === 配置外部子项目 === #
include(ExternalProject)

# 配置外部子项目 zlib-1.3.2 #
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(ZLIB_LIBRARY_NAME zsd.lib)
else()
	set(ZLIB_LIBRARY_NAME zs.lib)
endif()
set(ZLIB_SOURCE_DIR_NAME zlib-1.3.2)
set(ZLIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/zlib_install)
ExternalProject_Add(zlib
	SOURCE_DIR
	${CMAKE_CURRENT_SOURCE_DIR}/external/${ZLIB_SOURCE_DIR_NAME}

	CMAKE_ARGS
	-DCMAKE_INSTALL_PREFIX=${ZLIB_INSTALL_DIR}
	-DBUILD_SHARED_LIBS=OFF
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

# 配置外部子项目 libzip-1.11.4 #
set(LIBZIP_LIBRARY_NAME zip.lib)
set(LIBZIP_SOURCE_DIR_NAME libzip-1.11.4)
set(LIBZIP_INSTALL_DIR ${CMAKE_BINARY_DIR}/libzip_install)
ExternalProject_Add(libzip
	SOURCE_DIR
	${CMAKE_CURRENT_SOURCE_DIR}/external/${LIBZIP_SOURCE_DIR_NAME}

	CMAKE_ARGS
	-DCMAKE_INSTALL_PREFIX=${LIBZIP_INSTALL_DIR}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DBUILD_SHARED_LIBS=OFF
	-DBUILD_TOOLS=OFF
	-DBUILD_EXAMPLES=OFF
	-DBUILD_OSSFUZZ=OFF
	-DBUILD_DOC=OFF
	-DLIBZIP_DO_INSTALL=ON
	-DENABLE_BZIP2=OFF
	-DENABLE_LZMA=OFF
	-DENABLE_OPENSSL=OFF
	-DENABLE_ZSTD=OFF
	-DZLIB_LIBRARY=${ZLIB_INSTALL_DIR}/lib/${ZLIB_LIBRARY_NAME}
	-DZLIB_INCLUDE_DIR=${ZLIB_INSTALL_DIR}/include

	DEPENDS
	zlib
)


# === 配置预编译静态外部库 === #
# 配置 fltk-1.4.5 #
set(FLTK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/fltk-1.4.5)
set(FLTK_INCLUDE_DIR ${FLTK_ROOT_DIR}/include)
set(FLTK_LIBRARY_DIR ${FLTK_ROOT_DIR}/lib/fltk.lib)

# 检查 FLTK 库是否存在 #
if(NOT EXISTS ${FLTK_LIBRARY_DIR})
	message(FATAL_ERROR "FLTK library not found at ${FLTK_LIBRARY_DIR}")
endif()


# === 配置外部子项目依赖 === #
add_dependencies(jbytedom
	zlib
	libzip
)

add_dependencies(jbytedom_cli
	zlib
	libzip
)


# === 配置头文件搜索路径 === #
target_include_directories(jbytedom PRIVATE
	${ZLIB_INSTALL_DIR}/include
	${LIBZIP_INSTALL_DIR}/include
	${FLTK_INCLUDE_DIR}
)

target_include_directories(jbytedom_cli PRIVATE
	${ZLIB_INSTALL_DIR}/include
	${LIBZIP_INSTALL_DIR}/include
)


# === 配置链接库 === #
target_link_libraries(jbytedom PRIVATE
	${LIBZIP_INSTALL_DIR}/lib/${LIBZIP_LIBRARY_NAME}
	${ZLIB_INSTALL_DIR}/lib/${ZLIB_LIBRARY_NAME}
	${FLTK_LIBRARY_DIR}
	comctl32
	gdi32
	shell32
	ole32
	uuid
	ws2_32
	gdiplus
)

target_link_libraries(jbytedom_cli PRIVATE
	${LIBZIP_INSTALL_DIR}/lib/${LIBZIP_LIBRARY_NAME}
	${ZLIB_INSTALL_DIR}/lib/${ZLIB_LIBRARY_NAME}
)


# === 配置 GUI 子系统（隐藏控制台） === #
if(MSVC)
	target_link_options(jbytedom PRIVATE
		/SUBSYSTEM:WINDOWS
		/ENTRY:mainCRTStartup
	)
endif()


# === 定义编译器宏 === #
target_compile_definitions(jbytedom PRIVATE
	_CRT_SECURE_NO_WARNINGS
)

target_compile_definitions(jbytedom_cli PRIVATE
	_CRT_SECURE_NO_WARNINGS
)


# === 定义项目路径宏 === #
target_compile_definitions(jbytedom_cli PRIVATE
	PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
	PROJECT_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/resource"
)