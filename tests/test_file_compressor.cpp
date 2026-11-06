#include "code_generator.h"
#include "file_compressor.h"
#include "frequency_analyzer.h"
#include "huffman_tree.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void writeTempFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out.is_open());
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.close();
}

std::uint64_t getFileSizeOnDisk(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    assert(in.is_open());
    return static_cast<std::uint64_t>(in.tellg());
}

} // namespace

int main() {
    std::cout << "=== FileZipper: FileCompressor & .huf Format Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Empty file compression
    // -------------------------------------------------------------------------
    {
        const std::string inFile = "temp_empty_test.txt";
        const std::string outFile = "temp_empty_test.huf";
        writeTempFile(inFile, "");

        FileCompressor compressor;
        CompressionStats stats = compressor.compressFile(inFile, outFile);

        assert(stats.originalFileSize == 0);
        assert(stats.encodedBitCount == 0);
        assert(stats.paddingBitCount == 0);
        assert(stats.compressedFileSize == FileCompressor::kHeaderSizeBytes);
        assert(getFileSizeOnDisk(outFile) == FileCompressor::kHeaderSizeBytes);
        assert(stats.compressionRatio == 0.0);
        assert(stats.spaceSavedPercent == 0.0);

        // Verify .huf header fields on disk
        std::ifstream hufIn(outFile, std::ios::binary);
        HufHeader header = FileCompressor::readHeader(hufIn);
        assert(header.originalFileSize == 0);
        assert(header.encodedBitCount == 0);
        for (std::uint64_t freq : header.frequencies) {
            assert(freq == 0);
        }
        hufIn.close();

        std::remove(inFile.c_str());
        std::remove(outFile.c_str());
        std::cout << "[PASS] Test 1: Empty file compression and .huf header verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Single repeated character file ("AAAAAAAAAA" - 10 'A's)
    // -------------------------------------------------------------------------
    {
        const std::string inFile = "temp_single_char.txt";
        const std::string outFile = "temp_single_char.huf";
        const std::string content(10, 'A');
        writeTempFile(inFile, content);

        FileCompressor compressor;
        CompressionStats stats = compressor.compressFile(inFile, outFile);

        // 10 occurrences of 'A' with code "0" -> 10 meaningful bits -> 2 packed bytes (6 padding bits)
        assert(stats.originalFileSize == 10);
        assert(stats.encodedBitCount == 10);
        assert(stats.paddingBitCount == 6);
        assert(stats.compressedFileSize == FileCompressor::kHeaderSizeBytes + 2);
        assert(getFileSizeOnDisk(outFile) == stats.compressedFileSize);

        std::ifstream hufIn(outFile, std::ios::binary);
        HufHeader header = FileCompressor::readHeader(hufIn);
        assert(header.originalFileSize == 10);
        assert(header.encodedBitCount == 10);
        assert(header.frequencies[static_cast<unsigned char>('A')] == 10);

        // Since "0" repeated 10 times + 6 padding zeros = 16 zero-bits, both payload bytes must be 0x00
        char payload[2]{};
        assert(hufIn.read(payload, 2).gcount() == 2);
        assert(static_cast<unsigned char>(payload[0]) == 0x00);
        assert(static_cast<unsigned char>(payload[1]) == 0x00);
        hufIn.close();

        std::remove(inFile.c_str());
        std::remove(outFile.c_str());
        std::cout << "[PASS] Test 2: Single repeated character file ('A' x 10 -> 10 bits, 2 payload bytes) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Multiple different characters ("ABRACADABRA") and consistency
    //         between encoded bit count and generated codes
    // -------------------------------------------------------------------------
    {
        const std::string inFile = "temp_multi_char.txt";
        const std::string outFile = "temp_multi_char.huf";
        const std::string content = "ABRACADABRA";
        writeTempFile(inFile, content);

        // Compute expected bit count independently via FrequencyAnalyzer + HuffmanTree + CodeGenerator
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer(content);
        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());
        CodeGenerator codeGen;
        codeGen.generateCodes(tree.getRoot());
        const std::uint64_t expectedBits = codeGen.calculateTotalEncodedBits(analyzer.getFrequencies());
        assert(expectedBits == 23);

        FileCompressor compressor;
        CompressionStats stats = compressor.compressFile(inFile, outFile);

        assert(stats.originalFileSize == 11);
        assert(stats.encodedBitCount == expectedBits);
        assert(stats.paddingBitCount == 1); // 23 bits + 1 padding bit = 24 bits (3 bytes)
        assert(stats.compressedFileSize == FileCompressor::kHeaderSizeBytes + 3);
        assert(getFileSizeOnDisk(outFile) == stats.compressedFileSize);

        std::ifstream hufIn(outFile, std::ios::binary);
        HufHeader header = FileCompressor::readHeader(hufIn);
        assert(header.originalFileSize == 11);
        assert(header.encodedBitCount == 23);
        assert(header.frequencies == analyzer.getFrequencies());
        hufIn.close();

        std::remove(inFile.c_str());
        std::remove(outFile.c_str());
        std::cout << "[PASS] Test 3: Multi-character file ('ABRACADABRA' -> 23 bits) and header consistency verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Binary data containing null bytes (0x00) and high bytes (0xFF)
    // -------------------------------------------------------------------------
    {
        const std::string inFile = "temp_binary_data.bin";
        const std::string outFile = "temp_binary_data.huf";
        const std::string binaryContent({'\x00', '\xFF', '\x00', '\x80', '\x7F', '\x00', '\xFF', '\x01', '\x00', '\xFF'});
        writeTempFile(inFile, binaryContent);

        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer(binaryContent);
        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());
        CodeGenerator codeGen;
        codeGen.generateCodes(tree.getRoot());
        const std::uint64_t expectedBits = codeGen.calculateTotalEncodedBits(analyzer.getFrequencies());

        FileCompressor compressor;
        CompressionStats stats = compressor.compressFile(inFile, outFile);

        assert(stats.originalFileSize == 10);
        assert(stats.encodedBitCount == expectedBits);

        std::ifstream hufIn(outFile, std::ios::binary);
        HufHeader header = FileCompressor::readHeader(hufIn);
        assert(header.originalFileSize == 10);
        assert(header.encodedBitCount == expectedBits);
        assert(header.frequencies[0x00] == 4);
        assert(header.frequencies[0xFF] == 3);
        assert(header.frequencies[0x80] == 1);
        assert(header.frequencies[0x7F] == 1);
        assert(header.frequencies[0x01] == 1);
        hufIn.close();

        std::remove(inFile.c_str());
        std::remove(outFile.c_str());
        std::cout << "[PASS] Test 4: Binary data with null bytes (0x00) and 0xFF compressed and verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 5: Larger repetitive file demonstrating positive space savings
    // -------------------------------------------------------------------------
    {
        const std::string inFile = "temp_large_text.txt";
        const std::string outFile = "temp_large_text.huf";
        std::string largeText;
        for (int i = 0; i < 600; ++i) {
            largeText += "FileZipper uses Huffman Coding to compress repetitive text files efficiently! ";
        }
        writeTempFile(inFile, largeText);

        FileCompressor compressor;
        CompressionStats stats = compressor.compressFile(inFile, outFile);

        assert(stats.originalFileSize == largeText.size());
        assert(stats.compressedFileSize < stats.originalFileSize);
        assert(stats.compressionRatio < 0.65);
        assert(stats.spaceSavedPercent > 35.0);

        std::remove(inFile.c_str());
        std::remove(outFile.c_str());
        std::cout << "[PASS] Test 5: Large repetitive file compressed from "
                  << stats.originalFileSize << " bytes to " << stats.compressedFileSize
                  << " bytes (Space saved: " << stats.spaceSavedPercent << "%)." << std::endl;
    }

    std::cout << "All FileCompressor unit tests passed successfully!" << std::endl;
    return 0;
}
