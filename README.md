# FileZipper — Huffman-Based File Compression & Decompression Utility

**FileZipper** is a Data Structures and Algorithms (DSA) semester project written in modern **C++17** with a planned **Qt** desktop graphical user interface. It implements lossless file compression and decompression completely from scratch using **Huffman Coding**, without relying on external compression libraries.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Core DSA Concepts & Architecture](#core-dsa-concepts--architecture)
   - [1. Frequency Analysis](#1-frequency-analysis)
   - [2. Greedy Algorithm & Huffman Tree Construction](#2-greedy-algorithm--huffman-tree-construction)
   - [3. Role of the Min-Heap & Complexity Analysis](#3-role-of-the-min-heap--complexity-analysis)
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
- **Binary-Safe**: Uses `unsigned char` (`0x00`–`0xFF`) and 64-bit counters (`std::uint64_t`) to safely process both text and arbitrary binary files (including null bytes).
- **Incremental & Tested**: Every milestone is accompanied by automated unit tests managed via CMake and CTest.

---

## Core DSA Concepts & Architecture

### 1. Frequency Analysis
Before constructing a Huffman Tree, we must know how often each byte value appears in the input file.
- **Direct-Address Lookup Table (`std::array<std::uint64_t, 256>`)**: Because a byte has exactly $2^8 = 256$ possible values (`0` to `255`), `FrequencyAnalyzer` uses a fixed-size 256-entry array (`FrequencyTable`). Indexing by `unsigned char` provides $O(1)$ lookup and update time with zero hash collisions or dynamic allocation overhead.
- **Binary Mode Streaming**: Files are read in `std::ios::binary` mode using a 4 KB buffer (`char buffer[4096]`), ensuring newline characters (`\r`, `\n`), null bytes (`0x00`), and high-bit bytes (`0x80`–`0xFF`) are counted without corruption.

### 2. Greedy Algorithm & Huffman Tree Construction
David Huffman's algorithm is a classic **Greedy Algorithm** that builds an optimal prefix-free binary tree from the bottom up:
1. **Initialization**: Create a leaf `Node` for every unique byte with a non-zero frequency and insert it into a **Min-Heap**.
2. **Greedy Choice**: At each step, extract the **two nodes with the smallest frequencies** (`leftChild` and `rightChild`). Making the least frequent symbols the deepest nodes in the tree guarantees that more frequent symbols stay closer to the root.
3. **Subtree Merging**: Create a new internal parent `Node` whose frequency is `leftChild->frequency + rightChild->frequency`, attach the two extracted nodes as its `left` and `right` children, and push the parent back into the Min-Heap.
4. **Termination**: Repeat until only a single node remains in the Min-Heap; this node is the root of the Huffman Tree whose frequency equals the total bytes in the file.
5. **Edge Cases Handled**:
   - **Empty Input (0 bytes)**: Produces an empty tree (`root == nullptr`).
   - **Single Unique Byte (e.g., `"AAAAA"`)**: Produces a single-node tree where the root itself is the leaf node.
   - **Deterministic Tie-Breaking**: When two nodes have the exact same frequency, `CompareNode` breaks ties using their `character` value so the exact same tree topology is built across all platforms.

### 3. Role of the Min-Heap & Complexity Analysis
A **Min-Heap** (`std::priority_queue<Node*, std::vector<Node*>, CompareNode>`) allows the builder to repeatedly locate and extract the two minimum-frequency nodes efficiently.

Let $N$ be the total number of bytes in the input file and $U \le 256$ be the number of unique byte values with non-zero frequency:
- **Frequency Analysis Time**: $O(N)$ to scan all bytes in the input file once.
- **Heap Insertion**: Inserting $U$ initial leaf nodes takes $O(U \log U)$ time.
- **Tree Merging Loop**: Each of the $U - 1$ merge iterations performs two `pop()` operations and one `push()` operation on the Min-Heap, costing $O(\log U)$ per iteration, or $O(U \log U)$ total.
- **Total Time Complexity**: $O(N + U \log U)$. Since $U \le 256$ is bounded by a small constant, tree construction is near-instantaneous once the $O(N)$ file scan completes.
- **Space Complexity**: $O(U)$ auxiliary memory — the frequency table uses $256 \times 8 = 2\text{ KB}$, and the Huffman Tree contains at most $2U - 1 \le 511$ nodes.

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
├── CMakeLists.txt                # Root CMake build and test configuration
├── README.md                     # Project documentation
├── .gitignore                    # Ignored build artifacts and temporary files
├── include/
│   ├── huffman_node.h            # Huffman Tree Node declaration & comparator
│   ├── frequency_analyzer.h      # 256-byte binary frequency analyzer interface
│   └── huffman_tree.h            # Huffman Tree builder & RAII memory manager
├── src/
│   ├── huffman_node.cpp          # Huffman Tree Node implementation
│   ├── frequency_analyzer.cpp    # Binary file/stream/buffer frequency counting
│   └── huffman_tree.cpp          # Min-heap greedy tree construction
└── tests/
    ├── test_huffman_node.cpp     # Unit tests for Node & CompareNode
    └── test_huffman_tree.cpp     # Unit tests for FrequencyAnalyzer & HuffmanTree
```

---

## Development Roadmap & Milestones

- [x] **Milestone 1: Repository & Initial Project Setup**
  - Project folder hierarchy (`include/`, `src/`, `tests/`)
  - `Node` structure (`character`, `frequency`, `left`, `right`, constructor, `isLeaf()`, `CompareNode`, tree memory cleanup)
  - Initial `CMakeLists.txt` and `test_huffman_node` unit test suite
- [x] **Milestone 2: Frequency Counter & Huffman Tree Builder**
  - Binary-safe 256-entry byte frequency analyzer (`FrequencyAnalyzer`, `FrequencyTable`)
  - Greedy Min-Heap Huffman Tree construction (`HuffmanTree`) with deterministic tie-breaking and RAII memory management
  - Comprehensive unit tests (`test_huffman_tree`) for text files, repeated symbols, binary/null bytes, empty inputs, and single-symbol files
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
ctest --output-on-failure -V
# Or run the test binaries directly:
./test_huffman_node
./test_huffman_tree
```
