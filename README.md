# FileZipper — Huffman-Based File Compression & Decompression Utility

**FileZipper** is a Data Structures and Algorithms (DSA) Complex Computing Problem (CCP) project implemented in modern **C++17** with a **Qt 6** desktop graphical user interface. It performs lossless file compression and decompression completely from scratch using **Huffman Coding**, and packages into a standalone, double-clickable macOS application bundle (`FileZipper.app`).

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Features](#features)
3. [Core DSA Concepts & Architecture](#core-dsa-concepts--architecture)
   - [1. Frequency Analysis](#1-frequency-analysis)
   - [2. Greedy Algorithm & Huffman Tree Construction](#2-greedy-algorithm--huffman-tree-construction)
   - [3. Role of the Min-Heap & Complexity Analysis](#3-role-of-the-min-heap--complexity-analysis)
   - [4. Prefix-Free Code Generation (DFS Tree Traversal)](#4-prefix-free-code-generation-dfs-tree-traversal)
   - [5. Bitwise Stream Packing & Custom `.huf` Archive Format](#5-bitwise-stream-packing--custom-huf-archive-format)
   - [6. Bitstream Decoding & Lossless File Decompression](#6-bitstream-decoding--lossless-file-decompression)
4. [Requirements](#requirements)
5. [Build from Source](#build-from-source)
6. [Run Tests](#run-tests)
7. [Package as a Standalone macOS App (`FileZipper.app`)](#package-as-a-standalone-macos-app-filezipperapp)
8. [Launch & Install the App](#launch--install-the-app)
9. [How to Use FileZipper](#how-to-use-filezipper)
10. [Troubleshooting](#troubleshooting)
11. [Project Structure](#project-structure)

---

## Project Overview

Standard uncompressed files represent every byte using a fixed 8-bit width. **FileZipper** scans an input file in binary mode, counts the exact occurrence frequency of all 256 byte values (`0x00`–`0xFF`), and builds an optimal prefix-free binary tree (**Huffman Tree**) using a Min-Heap priority queue. High-frequency bytes receive short bit sequences (as short as 1 bit), while rare bytes receive longer bit sequences, reducing overall file size on text and repetitive binary data with 100% lossless recovery.

---

## Features

- **Lossless File Compression**: Compresses any text or binary file into a portable `.huf` archive.
- **Lossless File Decompression**: Reconstructs the exact Huffman Tree from the `.huf` header and restores the original file byte-for-byte.
- **Custom `.huf` Binary Format**: Uses a 4-byte `'HUF1'` magic signature, 64-bit little-endian file size and bit counters, and a 256-entry frequency table for cross-platform safety.
- **Modern Qt 6 Desktop GUI**: Dark-themed desktop interface with drag-and-drop file support, automatic operation mode detection, custom vector tree icon, and non-blocking background worker threads (`QtConcurrent`).
- **Live Compression Statistics**: Displays original file size, compressed/restored size, compression ratio, space saved percentage, and decoded bit counts.
- **Standalone macOS `.app` Bundle**: Self-contained `dist/FileZipper.app` bundled with Qt 6 frameworks and plugins (`libqcocoa.dylib`) so it can be launched directly from Finder or `/Applications` without a terminal or IDE.

---

## Core DSA Concepts & Architecture

### 1. Frequency Analysis
- **Direct-Address Lookup Table (`std::array<std::uint64_t, 256>`)**: `FrequencyAnalyzer` uses a fixed 256-entry array (`FrequencyTable`) indexed by `unsigned char` (`0..255`), providing $O(1)$ update and lookup time per byte.
- **Binary Mode Streaming**: Files are read in `std::ios::binary` mode using a 4 KB buffer (`char buffer[4096]`), preserving newline bytes (`\r`, `\n`), null bytes (`0x00`), and high-bit bytes (`0x80`–`0xFF`).

### 2. Greedy Algorithm & Huffman Tree Construction
1. **Initialization**: Create a leaf `Node` for every unique byte with a non-zero frequency and push it into a **Min-Heap**.
2. **Greedy Choice**: Repeatedly extract the two lowest-frequency nodes (`leftChild` and `rightChild`) and merge them under a new internal parent `Node` whose frequency is their sum.
3. **Deterministic Tie-Breaking**: When frequencies are equal, `CompareNode` breaks ties deterministically by `character` value so the exact same tree topology is reconstructed during decompression.
4. **Edge Cases**: Safely handles 0-byte empty files (`root == nullptr`) and single-symbol files (`root->isLeaf() == true`).

### 3. Role of the Min-Heap & Complexity Analysis
Let $N$ be the total bytes in the input file and $U \le 256$ be the number of unique bytes:
- **Frequency Analysis**: $O(N)$ time, $O(1)$ auxiliary space ($2\text{ KB}$ table).
- **Tree Construction**: $O(U \log U)$ time via `std::priority_queue<Node*, std::vector<Node*>, CompareNode>`, with at most $2U - 1 \le 511$ tree nodes.

### 4. Prefix-Free Code Generation (DFS Tree Traversal)
`CodeGenerator` traverses the Huffman Tree via recursive **Depth-First Search (DFS)**, appending `'0'` on left branches and `'1'` on right branches (`CodeTable = std::array<std::string, 256>`). Single-symbol trees assign `"0"` (1 bit per symbol).

#### Example Code Table (`"ABRACADABRA"` — 11 bytes / 88 raw bits)
| Character | Frequency | Huffman Code | Code Length (bits) | Total Bits (`Freq × Len`) |
| :---: | :---: | :---: | :---: | :---: |
| `'A'` | 5 | `"0"` | 1 | 5 |
| `'R'` | 2 | `"10"` | 2 | 4 |
| `'B'` | 2 | `"110"` | 3 | 6 |
| `'C'` | 1 | `"1110"` | 4 | 4 |
| `'D'` | 1 | `"1111"` | 4 | 4 |
| **Total** | **11 bytes** | — | — | **23 bits (73.9% reduction vs. 88 bits)** |

### 5. Bitwise Stream Packing & Custom `.huf` Archive Format
`BitWriter` packs individual `'0'` and `'1'` bits MSB-first into 8-bit bytes and pads the final partial byte with trailing zeros. `FileCompressor` writes the portable 2068-byte little-endian header followed by the packed bitstream:

| Offset (Bytes) | Size (Bytes) | Field | Type | Description |
| :---: | :---: | :--- | :--- | :--- |
| `0` | `4` | **Magic Signature** | `char[4]` | `'H'`, `'U'`, `'F'`, `'1'` identifier |
| `4` | `8` | **Original File Size** | `uint64_t` (LE) | Exact uncompressed size in bytes |
| `12` | `2048` | **Frequency Table** | `uint64_t[256]` (LE) | Occurrence counts for all 256 byte values (`0x00`–`0xFF`) |
| `2060` | `8` | **Encoded Bit Count** | `uint64_t` (LE) | Total number of meaningful (unpadded) encoded bits |
| `2068` | $\lceil\text{bits}/8\rceil$ | **Compressed Bitstream** | `uint8_t[]` | MSB-first packed Huffman codes (0-padded in final byte) |

### 6. Bitstream Decoding & Lossless File Decompression
`FileDecompressor` validates the `'HUF1'` header and frequency sum, rebuilds the Huffman Tree, and uses `BitReader` to walk from `root` to leaf (`0` = left, `1` = right) for exactly `encodedBitCount` meaningful bits, ignoring trailing zero-padding bits.

---

## Requirements

- **Operating System**: macOS (Apple Silicon or Intel)
- **C++ Compiler**: AppleClang / Clang / GCC supporting **C++17** (Xcode Command Line Tools: `xcode-select --install`)
- **Build System**: **CMake** `>= 3.16`
- **GUI Framework**: **Qt 6** (`qtbase` `>= 6.2`, tested with Qt `6.12.0`)

Install CMake and Qt 6 on macOS using [Homebrew](https://brew.sh):
```bash
brew install cmake qtbase
```

---

## Build from Source

```bash
# 1. Clone the repository
git clone https://github.com/ilaybaakram/FileZipper.git
cd FileZipper

# 2. Configure and build in Release mode
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This builds:
- `libhuffman_core.a` — The standalone C++17 Huffman compression/decompression engine
- `build/FileZipper.app` — The macOS GUI application bundle
- 8 automated unit and GUI integration test executables

---

## Run Tests

Run all 8 unit and GUI integration test suites using CTest:
```bash
ctest --test-dir build --output-on-failure -V
```

---

## Package as a Standalone macOS App (`FileZipper.app`)

To bundle all required Qt 6 frameworks (`QtCore`, `QtGui`, `QtWidgets`, `QtConcurrent`, `QtDBus`), image/style plugins, and the macOS Cocoa platform plugin (`libqcocoa.dylib`) into a self-contained `.app` inside `dist/FileZipper.app`:

```bash
./scripts/package_macos.sh
```

What `./scripts/package_macos.sh` does:
1. Configures and builds `FileZipper.app` in `Release` mode.
2. Copies a clean bundle to `dist/FileZipper.app`.
3. Runs `macdeployqt dist/FileZipper.app -always-overwrite` to embed Qt 6 frameworks, plugins, and `Resources/qt.conf` (`@executable_path/../Frameworks`).
4. Applies an ad-hoc code signature (`codesign --force --deep --sign - dist/FileZipper.app`) so macOS allows local execution.

---

## Launch & Install the App

### Launch from Finder
1. Open **Finder** and navigate to your project folder: `~/Documents/CCP/dist/`
2. Double-click **`FileZipper.app`** (shown as **FileZipper**).

*(Or open the `dist` folder directly from Terminal with `open dist` and double-click `FileZipper.app`, or launch via `open dist/FileZipper.app`.)*

### Install into `/Applications`
To install **FileZipper** system-wide like any standard Mac app:
- **Via Finder**: Drag `FileZipper.app` from `dist/` into your **Applications** folder.
- **Via Terminal**:
  ```bash
  cp -R dist/FileZipper.app /Applications/
  ```
You can then launch **FileZipper** anytime from **Applications** or **Spotlight Search (Cmd + Space)**.

---

## How to Use FileZipper

1. **Select a File**:
   - Drag and drop any file directly onto the **File Selection** card, or click **Browse File...** to pick a file from Finder.
   - The card displays the file's name, full path, and formatted byte size badge.
2. **Compress a File**:
   - When you select a normal file (e.g., `notes.txt`), **Compress File (Create .huf Archive)** is selected automatically.
   - Review or customize the **Output Folder** and suggested **Output Filename** (`notes.txt.huf`).
   - Click **Compress File**.
3. **Decompress a `.huf` Archive**:
   - When you select a `.huf` archive (e.g., `notes.txt.huf`), **Decompress Archive (Restore from .huf)** is selected automatically and a safe output name (such as `notes.txt` or `restored_notes.txt`) is suggested.
   - Click **Decompress Archive**.
4. **Understand the Results**:
   - **Original Size / Compressed Size**: Exact byte counts before and after compression.
   - **Compression Ratio**: `Compressed Size / Original Size` expressed as a percentage.
   - **Space Saved**: Percentage of bytes saved (`100% - Compression Ratio`). *(Note: Very small files under ~2 KB may show `+header` overhead because the `.huf` header stores the 2068-byte frequency table; larger text or repetitive files typically save 35%–70%+).*
   - Click **Open Output Folder** on the green completion banner to view the output file in Finder.

---

## Troubleshooting

- **macOS Gatekeeper ("FileZipper cannot be opened because the developer cannot be verified")**:
  If sharing `FileZipper.app` with teammates or your professor over AirDrop/email/zip, macOS quarantine may flag unsigned/ad-hoc apps. Fix by right-clicking `FileZipper.app` → **Open**, or run:
  ```bash
  xattr -cr /path/to/FileZipper.app
  codesign --force --deep --sign - /path/to/FileZipper.app
  ```
- **Qt Platform Plugin Error ("Could not find the Qt platform plugin 'cocoa'")**:
  Ensure you run `./scripts/package_macos.sh` (which runs `macdeployqt` to embed `Contents/PlugIns/platforms/libqcocoa.dylib` and `Contents/Resources/qt.conf`).
- **CMake Cannot Find Qt6 (`Could not find a package configuration file provided by "Qt6"`)**:
  Install `qtbase` via Homebrew (`brew install qtbase`) or pass your Qt path explicitly:
  ```bash
  cmake -S . -B build -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qtbase"
  ```

---

## Project Structure

```text
FileZipper/
├── CMakeLists.txt                  # Root CMake build, MACOSX_BUNDLE, and CTest config
├── README.md                       # Project documentation
├── .gitignore                      # Ignored build/dist artifacts and temporary files
├── scripts/
│   └── package_macos.sh            # Automated standalone FileZipper.app bundler
├── include/
│   ├── huffman_node.h              # Huffman Tree Node declaration & comparator
│   ├── frequency_analyzer.h        # 256-byte binary frequency analyzer interface
│   ├── huffman_tree.h              # Huffman Tree builder & RAII memory manager
│   ├── code_generator.h            # Prefix-free binary code table generator
│   ├── bit_writer.h                # MSB-first bit-packing writer
│   ├── file_compressor.h           # .huf archive serializer & file compressor
│   ├── bit_reader.h                # MSB-first bit reader with meaningful-bit limit
│   ├── file_decompressor.h         # .huf archive validator & file decompressor
│   └── main_window.h               # Qt 6 MainWindow & DropZoneFrame declarations
├── src/
│   ├── huffman_node.cpp            # Huffman Tree Node implementation
│   ├── frequency_analyzer.cpp      # Binary file/stream/buffer frequency counting
│   ├── huffman_tree.cpp            # Min-heap greedy tree construction
│   ├── code_generator.cpp          # Recursive DFS code generation & bit metrics
│   ├── bit_writer.cpp              # Bit-packing & partial-byte padding implementation
│   ├── file_compressor.cpp         # Two-pass Huffman file compression & .huf header I/O
│   ├── bit_reader.cpp              # Bit extraction & padding-aware stream reader
│   ├── file_decompressor.cpp       # Tree reconstruction & bitstream decoding engine
│   ├── main_window.cpp             # Dark-theme Qt 6 GUI & QtConcurrent integration
│   └── main.cpp                    # Application entry point (GUI + CLI verification flags)
└── tests/
    ├── test_huffman_node.cpp       # Unit tests for Node & CompareNode
    ├── test_huffman_tree.cpp       # Unit tests for FrequencyAnalyzer & HuffmanTree
    ├── test_code_generator.cpp     # Unit tests for CodeGenerator & prefix-free property
    ├── test_bit_writer.cpp         # Unit tests for BitWriter full/partial byte packing
    ├── test_file_compressor.cpp    # Unit tests for FileCompressor & .huf format
    ├── test_bit_reader.cpp         # Unit tests for BitReader & meaningful bit limits
    ├── test_file_decompressor.cpp  # Round-trip & malformed-archive unit tests
    └── test_gui.cpp                # Qt 6 GUI launch, compress, decompress & error tests
```
