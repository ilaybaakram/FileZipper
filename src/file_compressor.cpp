#include "file_compressor.h"

#include "bit_writer.h"
#include "code_generator.h"
#include "huffman_tree.h"

#include <fstream>
#include <stdexcept>

namespace {

// Writes a 64-bit unsigned integer in portable little-endian byte order
void writeUint64LE(std::ostream& output, std::uint64_t value) {
    unsigned char bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xFFU);
    }
    output.write(reinterpret_cast<const char*>(bytes), 8);
}

// Reads a 64-bit unsigned integer in portable little-endian byte order
std::uint64_t readUint64LE(std::istream& input) {
    unsigned char bytes[8]{};
    if (!input.read(reinterpret_cast<char*>(bytes), 8)) {
        throw std::runtime_error("Unexpected end of stream while reading 64-bit integer from .huf header");
    }
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= (static_cast<std::uint64_t>(bytes[i]) << (i * 8));
    }
    return value;
}

} // namespace

void FileCompressor::writeHeader(
    std::ostream& output,
    std::uint64_t originalFileSize,
    const FrequencyTable& frequencies,
    std::uint64_t encodedBitCount
) {
    // 1. Write 4-byte magic signature ('H', 'U', 'F', '1')
    output.write(kMagic, sizeof(kMagic));

    // 2. Write 8-byte original uncompressed file size
    writeUint64LE(output, originalFileSize);

    // 3. Write all 256 frequency table entries (256 * 8 = 2048 bytes)
    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        writeUint64LE(output, frequencies[i]);
    }

    // 4. Write 8-byte total number of meaningful encoded bits
    writeUint64LE(output, encodedBitCount);

    if (!output.good()) {
        throw std::runtime_error("Failed to write .huf header to output stream");
    }
}

HufHeader FileCompressor::readHeader(std::istream& input) {
    HufHeader header{};

    if (!input.read(header.magic, sizeof(header.magic))) {
        throw std::runtime_error("Failed to read magic signature from .huf file");
    }

    for (int i = 0; i < 4; ++i) {
        if (header.magic[i] != kMagic[i]) {
            throw std::runtime_error("Invalid .huf file: magic signature mismatch");
        }
    }

    header.originalFileSize = readUint64LE(input);

    for (std::size_t i = 0; i < header.frequencies.size(); ++i) {
        header.frequencies[i] = readUint64LE(input);
    }

    header.encodedBitCount = readUint64LE(input);

    return header;
}

CompressionStats FileCompressor::compressFile(
    const std::string& inputFilePath,
    const std::string& outputFilePath
) const {
    // Step 1: Analyze byte frequencies in the input file
    FrequencyAnalyzer analyzer;
    analyzer.analyzeFile(inputFilePath);

    const FrequencyTable& frequencies = analyzer.getFrequencies();
    const std::uint64_t originalFileSize = analyzer.getTotalBytes();

    // Step 2: Build the Huffman Tree from the frequency table
    HuffmanTree tree;
    tree.buildFromFrequencies(frequencies);

    // Step 3: Generate prefix-free Huffman codes and compute expected bit count
    CodeGenerator codeGen;
    codeGen.generateCodes(tree.getRoot());
    const std::uint64_t expectedEncodedBits = codeGen.calculateTotalEncodedBits(frequencies);

    // Step 4: Open output .huf file in binary mode and write the header
    std::ofstream outFile(outputFilePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open output file for compression: " + outputFilePath);
    }

    writeHeader(outFile, originalFileSize, frequencies, expectedEncodedBits);

    // Step 5: Encode the input file bitstream using BitWriter
    BitWriter bitWriter(outFile);
    std::uint8_t paddingBits = 0;

    if (originalFileSize > 0) {
        std::ifstream inFile(inputFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            throw std::runtime_error("Failed to reopen input file for bitstream encoding: " + inputFilePath);
        }

        char buffer[4096];
        std::uint64_t totalBytesRead = 0;

        while (inFile.read(buffer, sizeof(buffer)) || inFile.gcount() > 0) {
            std::streamsize bytesRead = inFile.gcount();
            for (std::streamsize i = 0; i < bytesRead; ++i) {
                unsigned char byte = static_cast<unsigned char>(buffer[i]);
                bitWriter.writeBits(codeGen.getCode(byte));
            }
            totalBytesRead += static_cast<std::uint64_t>(bytesRead);
            if (inFile.eof()) {
                break;
            }
        }

        if (inFile.bad() || totalBytesRead != originalFileSize) {
            throw std::runtime_error("I/O error or size mismatch during encoding of: " + inputFilePath);
        }

        paddingBits = bitWriter.flush();
    }

    outFile.flush();
    if (!outFile.good()) {
        throw std::runtime_error("Failed to finalize output .huf file: " + outputFilePath);
    }

    const std::uint64_t actualMeaningfulBits = bitWriter.getMeaningfulBitsWritten();
    if (actualMeaningfulBits != expectedEncodedBits) {
        throw std::runtime_error("Encoded bit count mismatch between CodeGenerator and BitWriter");
    }

    const std::uint64_t compressedFileSize = kHeaderSizeBytes + bitWriter.getBytesWritten();

    // Step 6: Compute compression statistics safely (handles empty files & expansion)
    double compressionRatio = 0.0;
    double spaceSavedPercent = 0.0;
    if (originalFileSize > 0) {
        compressionRatio = static_cast<double>(compressedFileSize) / static_cast<double>(originalFileSize);
        spaceSavedPercent = (1.0 - compressionRatio) * 100.0;
    }

    return CompressionStats{
        originalFileSize,
        compressedFileSize,
        actualMeaningfulBits,
        paddingBits,
        compressionRatio,
        spaceSavedPercent
    };
}
