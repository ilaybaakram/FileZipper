#include "file_decompressor.h"

#include "bit_reader.h"
#include "code_generator.h"
#include "file_compressor.h"
#include "huffman_tree.h"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

DecompressionStats FileDecompressor::decompressFile(
    const std::string& inputFilePath,
    const std::string& outputFilePath
) const {
    // Step 1: Open the compressed .huf input file in binary mode and determine its size
    std::ifstream inFile(inputFilePath, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        throw std::runtime_error("Failed to open input .huf file for decompression: " + inputFilePath);
    }

    const std::streampos endPos = inFile.tellg();
    if (endPos < 0) {
        throw std::runtime_error("Failed to determine size of input .huf file: " + inputFilePath);
    }
    const std::uint64_t compressedFileSize = static_cast<std::uint64_t>(endPos);
    inFile.seekg(0, std::ios::beg);

    // Step 2: Read and validate the 2068-byte .huf header (verifies 'HUF1' magic signature)
    if (compressedFileSize < FileCompressor::kHeaderSizeBytes) {
        throw std::runtime_error("Corrupted or truncated .huf file: smaller than header size");
    }

    const HufHeader header = FileCompressor::readHeader(inFile);

    // Step 3: Validate consistency of the frequency table against originalFileSize
    std::uint64_t frequencySum = 0;
    for (std::uint64_t count : header.frequencies) {
        if ( std::numeric_limits<std::uint64_t>::max() - frequencySum < count ) {
            throw std::runtime_error("Corrupted .huf header: frequency table sum overflow");
        }
        frequencySum += count;
    }

    if (frequencySum != header.originalFileSize) {
        throw std::runtime_error(
            "Corrupted .huf header: sum of frequency table does not match originalFileSize"
        );
    }

    // Handle empty file edge case validation
    if (header.originalFileSize == 0) {
        if (header.encodedBitCount != 0) {
            throw std::runtime_error(
                "Corrupted .huf header: originalFileSize is 0 but encodedBitCount is non-zero"
            );
        }

        std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open output file for decompression: " + outputFilePath);
        }
        outFile.flush();
        if (!outFile.good()) {
            throw std::runtime_error("Failed to finalize empty decompressed file: " + outputFilePath);
        }

        return DecompressionStats{compressedFileSize, 0, 0};
    }

    // Step 4: Reconstruct the Huffman Tree and verify expected encoded bit count
    HuffmanTree tree;
    tree.buildFromFrequencies(header.frequencies);

    const Node* root = tree.getRoot();
    if (root == nullptr) {
        throw std::runtime_error("Internal error: Huffman tree root is null for non-empty file");
    }

    CodeGenerator codeGen;
    codeGen.generateCodes(root);
    const std::uint64_t expectedBitCount = codeGen.calculateTotalEncodedBits(header.frequencies);

    if (header.encodedBitCount != expectedBitCount) {
        throw std::runtime_error(
            "Corrupted .huf header: encodedBitCount does not match reconstructed Huffman tree"
        );
    }

    // Check that the file contains enough payload bytes for header.encodedBitCount
    const std::uint64_t requiredPayloadBytes = (header.encodedBitCount + 7ULL) / 8ULL;
    const std::uint64_t availablePayloadBytes = compressedFileSize - FileCompressor::kHeaderSizeBytes;
    if (availablePayloadBytes < requiredPayloadBytes) {
        throw std::runtime_error("Truncated .huf bitstream: insufficient payload bytes on disk");
    }

    // Step 5: Open output file for writing reconstructed bytes
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open output file for decompression: " + outputFilePath);
    }

    // Step 6: Decode bitstream using BitReader and buffered output
    BitReader bitReader(inFile, header.encodedBitCount);
    std::vector<char> outBuffer;
    outBuffer.reserve(4096);

    std::uint64_t bytesRestored = 0;

    if (root->isLeaf()) {
        // Special Case: Single unique byte in the entire file.
        // CodeGenerator assigns "0" (1 bit per symbol) for single-leaf trees.
        while (bytesRestored < header.originalFileSize) {
            int bit = 0;
            if (!bitReader.readBit(bit)) {
                throw std::runtime_error("Truncated .huf bitstream while decoding single-symbol file");
            }
            if (bit != 0) {
                throw std::runtime_error(
                    "Corrupted .huf bitstream: expected '0' bit for single-symbol tree"
                );
            }
            outBuffer.push_back(static_cast<char>(root->character));
            if (outBuffer.size() == 4096) {
                outFile.write(outBuffer.data(), static_cast<std::streamsize>(outBuffer.size()));
                if (!outFile.good()) {
                    throw std::runtime_error("Failed to write decompressed data to: " + outputFilePath);
                }
                outBuffer.clear();
            }
            ++bytesRestored;
        }
    } else {
        // General Case: Multi-symbol tree traversal (0 = left, 1 = right)
        const Node* current = root;
        while (bytesRestored < header.originalFileSize) {
            int bit = 0;
            if (!bitReader.readBit(bit)) {
                throw std::runtime_error(
                    "Truncated .huf bitstream: reached end of encoded bits before restoring full file"
                );
            }

            current = (bit == 0) ? current->left : current->right;
            if (current == nullptr) {
                throw std::runtime_error("Corrupted .huf bitstream: traversal reached null tree branch");
            }

            if (current->isLeaf()) {
                outBuffer.push_back(static_cast<char>(current->character));
                if (outBuffer.size() == 4096) {
                    outFile.write(outBuffer.data(), static_cast<std::streamsize>(outBuffer.size()));
                    if (!outFile.good()) {
                        throw std::runtime_error("Failed to write decompressed data to: " + outputFilePath);
                    }
                    outBuffer.clear();
                }
                ++bytesRestored;
                current = root;
            }
        }
    }

    // Flush any remaining buffered output bytes
    if (!outBuffer.empty()) {
        outFile.write(outBuffer.data(), static_cast<std::streamsize>(outBuffer.size()));
    }
    outFile.flush();
    if (!outFile.good()) {
        throw std::runtime_error("Failed to flush decompressed output file: " + outputFilePath);
    }

    // Step 7: Final consistency check between decoded bits and header.encodedBitCount
    if (bitReader.getBitsRead() != header.encodedBitCount) {
        throw std::runtime_error(
            "Decompression bit count mismatch: consumed bits do not equal header encodedBitCount"
        );
    }

    return DecompressionStats{
        compressedFileSize,
        bytesRestored,
        bitReader.getBitsRead()
    };
}
