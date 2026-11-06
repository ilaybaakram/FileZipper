# FileZipper — Huffman-Based File Compression & Decompression Utility

**FileZipper** is a Data Structures and Algorithms (DSA) semester project written in modern **C++17** with a planned **Qt** desktop graphical user interface. It implements lossless file compression and decompression completely from scratch using **Huffman Coding**, without relying on external compression libraries.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [How Huffman Coding Works](#how-huffman-coding-works)
3. [Technology Stack](#technology-stack)
4. [Project Structure](#project-structure)
5. [Development Roadmap & Milestones](#development-roadmap--milestones)
6. [Building and Running Tests](#building-and-running-tests)

---

## Project Overview

Standard uncompressed files store every byte using a fixed-length 8-bit representation. **FileZipper** analyzes byte frequencies in an input file and constructs an optimal prefix-free binary tree (**Huffman Tree**). Frequently occurring bytes are assigned shorter bit sequences, while rarer bytes receive longer bit sequences, significantly reducing overall file size on repetitive or text-heavy data.

### Key Design Goals
- **From-Scratch DSA Implementation**: Custom tree nodes, priority queue integration, prefix code table generation, and bitwise file I/O streams.
- **Clean Architecture**: Strict separation between the core C++ compression engine (`huffman_core`) and the presentation layer (CLI / Qt GUI).
- **Binary-Safe**: Uses `unsigned char` and `std::size_t` to safely handle arbitrary bytes and large files.
- **Incremental & Tested**: Every component is accompanied by automated unit tests managed via CMake and CTest.

---

## How Huffman Coding Works

1. **Frequency Analysis**: Scan the input file and count the occurrences of each unique byte (`0x00`–`0xFF`).
2. **Priority Queue (Min-Heap)**: Insert each unique byte as a leaf `Node` into a min-heap ordered by frequency.
3. **Tree Construction**: Repeatedly extract the two lowest-frequency nodes and merge them under a new internal `Node` whose frequency is their sum, until a single root remains.
4. **Code Generation**: Traverse the Huffman Tree from root to leaves (`0` for left branch, `1` for right branch) to assign unique prefix-free bitstrings to each byte.
5. **Bitwise Encoding (Compression)**: Write a serialized tree/frequency header followed by the packed variable-length bit codes into the compressed `.huff` output file.
6. **Bitwise Decoding (Decompression)**: Reconstruct the Huffman Tree from the header and traverse the tree bit-by-bit from root to leaf to restore the exact original file bytes.

---

## Technology Stack

| Component | Technology |
| :--- | :--- |
| **Core Language** | C++17 |
| **Desktop GUI** | Qt (planned for GUI milestone) |
| **Build System** | CMake (>= 3.16) & CTest |
| **Version Control** | Git & GitHub |

---

## Project Structure

```text
FileZipper/
├── CMakeLists.txt              # Root CMake build and test configuration
├── README.md                   # Project documentation
├── .gitignore                  # Ignored build artifacts and temporary files
├── include/
│   └── huffman_node.h          # Huffman Tree Node declaration & comparator
├── src/
│   └── huffman_node.cpp        # Huffman Tree Node implementation
└── tests/
    └── test_huffman_node.cpp   # Unit tests for Node construction & min-heap ordering
```

---

## Development Roadmap & Milestones

- [x] **Milestone 1: Repository & Initial Project Setup**
  - Project folder hierarchy (`include/`, `src/`, `tests/`)
  - `Node` structure (`character`, `frequency`, `left`, `right`, constructor, `isLeaf()`, `CompareNode`, tree memory cleanup)
  - Initial `CMakeLists.txt` and `test_huffman_node` unit test suite
- [ ] **Milestone 2: Frequency Counter & Huffman Tree Builder**
  - Byte frequency table analysis and min-heap tree construction
- [ ] **Milestone 3: Prefix Code Generator**
  - Recursive tree traversal to generate bitstring lookup tables
- [ ] **Milestone 4: Bitwise File I/O & Compression Engine**
  - Bit-packing writer/reader, file header serialization, and `.huff` encoder
- [ ] **Milestone 5: Decompression Engine & End-to-End Verification**
  - Header parsing, tree reconstruction, bit-stream decoding, and round-trip integrity tests
- [ ] **Milestone 6: Qt Desktop GUI Integration**
  - Modern desktop interface for selecting files, viewing compression ratios, and compressing/decompressing archives

---

## Building and Running Tests

### Prerequisites
- A C++17 compatible compiler (`clang++`, `g++`, or MSVC)
- **CMake** (version 3.16 or newer)

### Configure and Build
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Run Unit Tests
```bash
ctest --output-on-failure
# Or run the test binary directly:
./test_huffman_node
```
