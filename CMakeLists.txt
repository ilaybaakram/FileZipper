cmake_minimum_required(VERSION 3.16)

project(FileZipper
    VERSION 1.0.0
    DESCRIPTION "Huffman-Based File Compression and Decompression Utility"
    LANGUAGES CXX
)

# Enforce modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE / clangd tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent macOS Spotlight from indexing intermediate .app bundles inside the build directory
if(APPLE)
    file(WRITE "${CMAKE_BINARY_DIR}/.metadata_never_index" "")
endif()

# Enable strict compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Core DSA Engine Static Library (Independent of Qt)
# ============================================================================
add_library(huffman_core STATIC
    src/huffman_node.cpp
    src/frequency_analyzer.cpp
    src/huffman_tree.cpp
    src/code_generator.cpp
    src/bit_writer.cpp
    src/file_compressor.cpp
    src/bit_reader.cpp
    src/file_decompressor.cpp
)

target_include_directories(huffman_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ============================================================================
# Qt 6 / Qt 5 Discovery & Desktop GUI Target
# ============================================================================
if(APPLE AND EXISTS "/opt/homebrew/opt/qtbase")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qtbase")
elseif(APPLE AND EXISTS "/opt/homebrew/opt/qt")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets Concurrent REQUIRED)
    set(QT_PACKAGE_PREFIX Qt5)
else()
    set(QT_PACKAGE_PREFIX Qt6)
endif()

message(STATUS "Building FileZipper GUI with ${QT_PACKAGE_PREFIX}")

# Reusable GUI library (enables linking MainWindow into both FileZipper and test_gui)
add_library(filezipper_gui_lib STATIC
    include/main_window.h
    src/main_window.cpp
)

set_target_properties(filezipper_gui_lib PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_include_directories(filezipper_gui_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(filezipper_gui_lib PUBLIC
    huffman_core
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Gui
    ${QT_PACKAGE_PREFIX}::Widgets
    ${QT_PACKAGE_PREFIX}::Concurrent
)

# Desktop GUI Application Executable (built as a macOS .app bundle on Apple platforms)
set(APP_ICON_ICNS "${PROJECT_SOURCE_DIR}/assets/FileZipper.icns")
set(APP_ICON_PNG "${PROJECT_SOURCE_DIR}/assets/app_icon.png")
set_source_files_properties(${APP_ICON_ICNS} ${APP_ICON_PNG} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

add_executable(FileZipper MACOSX_BUNDLE
    src/main.cpp
    ${APP_ICON_ICNS}
    ${APP_ICON_PNG}
)

set_target_properties(FileZipper PROPERTIES
    AUTOMOC ON
    MACOSX_BUNDLE_BUNDLE_NAME "FileZipper"
    MACOSX_BUNDLE_GUI_IDENTIFIER "edu.dsa.filezipper"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "FileZipper.icns"
    MACOSX_BUNDLE_INFO_STRING "File Zipper — Huffman-Based Lossless File Compression Utility"
    MACOSX_BUNDLE_COPYRIGHT "DSA Semester Project"
)

target_link_libraries(FileZipper PRIVATE
    filezipper_gui_lib
)

# ============================================================================
# Automated Unit & GUI Integration Tests (CTest)
# ============================================================================
enable_testing()

add_executable(test_huffman_node
    tests/test_huffman_node.cpp
)
target_link_libraries(test_huffman_node PRIVATE huffman_core)
add_test(NAME HuffmanNodeTest COMMAND test_huffman_node)

add_executable(test_huffman_tree
    tests/test_huffman_tree.cpp
)
target_link_libraries(test_huffman_tree PRIVATE huffman_core)
add_test(NAME HuffmanTreeTest COMMAND test_huffman_tree)

add_executable(test_code_generator
    tests/test_code_generator.cpp
)
target_link_libraries(test_code_generator PRIVATE huffman_core)
add_test(NAME CodeGeneratorTest COMMAND test_code_generator)

add_executable(test_bit_writer
    tests/test_bit_writer.cpp
)
target_link_libraries(test_bit_writer PRIVATE huffman_core)
add_test(NAME BitWriterTest COMMAND test_bit_writer)

add_executable(test_file_compressor
    tests/test_file_compressor.cpp
)
target_link_libraries(test_file_compressor PRIVATE huffman_core)
add_test(NAME FileCompressorTest COMMAND test_file_compressor)

add_executable(test_bit_reader
    tests/test_bit_reader.cpp
)
target_link_libraries(test_bit_reader PRIVATE huffman_core)
add_test(NAME BitReaderTest COMMAND test_bit_reader)

add_executable(test_file_decompressor
    tests/test_file_decompressor.cpp
)
target_link_libraries(test_file_decompressor PRIVATE huffman_core)
add_test(NAME FileDecompressorTest COMMAND test_file_decompressor)

add_executable(test_gui
    tests/test_gui.cpp
)
set_target_properties(test_gui PROPERTIES AUTOMOC ON)
target_link_libraries(test_gui PRIVATE filezipper_gui_lib)
add_test(NAME GuiIntegrationTest COMMAND test_gui)
