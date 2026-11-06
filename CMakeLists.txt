cmake_minimum_required(VERSION 3.16)

project(FileZipper
    VERSION 0.1.0
    DESCRIPTION "Huffman-Based File Compression and Decompression Utility"
    LANGUAGES CXX
)

# Enforce modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE / clangd tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable strict compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core DSA Engine Static Library
add_library(huffman_core STATIC
    src/huffman_node.cpp
)

target_include_directories(huffman_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Enable CTest for automated unit testing
enable_testing()

add_executable(test_huffman_node
    tests/test_huffman_node.cpp
)

target_link_libraries(test_huffman_node PRIVATE huffman_core)

add_test(NAME HuffmanNodeTest COMMAND test_huffman_node)
