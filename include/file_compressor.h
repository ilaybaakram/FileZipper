#ifndef FILE_COMPRESSOR_H
#define FILE_COMPRESSOR_H

#include "frequency_analyzer.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

/**
 * @brief Holds parsed metadata from a custom .huf archive header.
 */
struct HufHeader {
    char magic[4];                  ///< 4-byte format signature ('H', 'U', 'F', '1')
    std::uint64_t originalFileSize; ///< Uncompressed input size in bytes
    FrequencyTable frequencies;     ///< 256-entry frequency table (uint64_t per byte)
    std::uint64_t encodedBitCount;  ///< Total number of meaningful encoded bits in payload
};

/**
 * @brief Reports compression metrics after encoding a file or stream.
 */
struct CompressionStats {
    std::uint64_t originalFileSize;   ///< Size of the original input file in bytes
    std::uint64_t compressedFileSize; ///< Total size of the .huf file (header + bitstream) in bytes
    std::uint64_t encodedBitCount;    ///< Total number of meaningful Huffman-encoded bits
    std::uint8_t paddingBitCount;     ///< Trailing zero-padding bits in the final byte (0..7)
    double compressionRatio;          ///< Ratio: compressedFileSize / originalFileSize (0.0 if empty)
    double spaceSavedPercent;         ///< Percentage saved: (1.0 - compressionRatio) * 100.0
};

/**
 * @brief Compresses input files into the portable custom .huf binary archive format.
 *
 * Custom .huf Binary Format Specification (Little-Endian, Fixed-Width Fields):
 * ----------------------------------------------------------------------------
 * Offset   Size (Bytes)   Type                      Description
 * 0        4              char[4]                   Magic signature: 'H', 'U', 'F', '1'
 * 4        8              uint64_t (LE)             Original uncompressed file size in bytes
 * 12       2048           uint64_t[256] (LE)        Frequency table for all 256 byte values (0..255)
 * 2060     8              uint64_t (LE)             Total number of meaningful encoded bits
 * 2068     ceil(bits/8)   uint8_t[]                 Packed Huffman bitstream (MSB-first, 0-padded)
 * ----------------------------------------------------------------------------
 * Total Header Size = 4 + 8 + 2048 + 8 = 2068 bytes.
 */
class FileCompressor {
public:
    static constexpr char kMagic[4] = {'H', 'U', 'F', '1'};
    static constexpr std::uint64_t kHeaderSizeBytes = 4ULL + 8ULL + (256ULL * 8ULL) + 8ULL; // 2068 bytes

    FileCompressor() = default;

    /**
     * @brief Compresses an input file on disk and writes the .huf output file.
     *
     * Workflow:
     * 1. Analyzes the input file's 256 byte frequencies in binary mode.
     * 2. Builds the optimal Huffman Tree.
     * 3. Generates prefix-free Huffman codes for all occurring bytes.
     * 4. Writes the portable 2068-byte .huf header.
     * 5. Encodes the input file using BitWriter and pads the final byte.
     *
     * @param inputFilePath  Path to the source file to compress.
     * @param outputFilePath Path to the output .huf file.
     * @return CompressionStats Metrics including original size, compressed size, ratio, and space saved.
     * @throws std::runtime_error if a file cannot be opened or an I/O error occurs.
     */
    CompressionStats compressFile(const std::string& inputFilePath, const std::string& outputFilePath) const;

    /**
     * @brief Serializes a HufHeader into a binary output stream using portable
     *        little-endian fixed-width integers (no raw struct writes).
     */
    static void writeHeader(
        std::ostream& output,
        std::uint64_t originalFileSize,
        const FrequencyTable& frequencies,
        std::uint64_t encodedBitCount
    );

    /**
     * @brief Reads and validates a HufHeader from a binary input stream.
     *
     * @param input Binary input stream positioned at the beginning of a .huf file.
     * @return HufHeader Parsed header fields.
     * @throws std::runtime_error if the magic signature is invalid or the header is truncated.
     */
    static HufHeader readHeader(std::istream& input);
};

#endif // FILE_COMPRESSOR_H
