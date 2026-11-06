#ifndef FILE_DECOMPRESSOR_H
#define FILE_DECOMPRESSOR_H

#include <cstdint>
#include <string>

/**
 * @brief Reports decompression metrics after restoring a file from a .huf archive.
 */
struct DecompressionStats {
    std::uint64_t compressedFileSize; ///< Size of the input .huf archive in bytes
    std::uint64_t restoredFileSize;   ///< Total number of uncompressed bytes reconstructed
    std::uint64_t decodedBitCount;    ///< Total number of meaningful bits consumed during decoding
};

/**
 * @brief Decompresses custom .huf binary archives back into their exact original files.
 *
 * Workflow:
 * 1. Opens the .huf file in binary mode and parses + validates the 2068-byte HufHeader.
 * 2. Verifies the 'HUF1' magic signature and checks that the frequency table sum
 *    matches originalFileSize.
 * 3. Reconstructs the deterministic Huffman Tree using HuffmanTree::buildFromFrequencies
 *    and verifies the expected encoded bit count via CodeGenerator.
 * 4. Decodes the packed bitstream MSB-first using BitReader (0 = left, 1 = right),
 *    writing decoded leaf bytes to the output file and stopping once originalFileSize
 *    bytes are restored.
 * 5. Verifies that both the restored byte count and consumed meaningful bit count
 *    match the header.
 */
class FileDecompressor {
public:
    FileDecompressor() = default;

    /**
     * @brief Decompresses an input .huf file and writes the reconstructed bytes to outputFilePath.
     *
     * @param inputFilePath  Path to the compressed .huf archive.
     * @param outputFilePath Path where the reconstructed original file will be written.
     * @return DecompressionStats Statistics describing compressed size, restored size, and decoded bits.
     * @throws std::runtime_error if the input/output file cannot be opened, or if the .huf header,
     *         frequency table, or bitstream is corrupted or truncated.
     */
    DecompressionStats decompressFile(
        const std::string& inputFilePath,
        const std::string& outputFilePath
    ) const;
};

#endif // FILE_DECOMPRESSOR_H
