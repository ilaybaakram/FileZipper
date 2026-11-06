#include "file_compressor.h"
#include "file_decompressor.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void writeFileBytes(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out.is_open());
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.close();
}

std::string readFileBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    assert(in.is_open());
    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::string buffer(static_cast<std::size_t>(size), '\0');
    if (size > 0) {
        assert(in.read(&buffer[0], size));
    }
    return buffer;
}

void verifyRoundTrip(const std::string& testName, const std::string& originalContent) {
    const std::string inPath = "temp_rt_in.bin";
    const std::string hufPath = "temp_rt_compressed.huf";
    const std::string outPath = "temp_rt_restored.bin";

    writeFileBytes(inPath, originalContent);

    FileCompressor compressor;
    CompressionStats compStats = compressor.compressFile(inPath, hufPath);
    assert(compStats.originalFileSize == originalContent.size());

    FileDecompressor decompressor;
    DecompressionStats decompStats = decompressor.decompressFile(hufPath, outPath);

    assert(decompStats.compressedFileSize == compStats.compressedFileSize);
    assert(decompStats.restoredFileSize == originalContent.size());
    assert(decompStats.decodedBitCount == compStats.encodedBitCount);

    const std::string restoredContent = readFileBytes(outPath);
    assert(restoredContent == originalContent);

    std::remove(inPath.c_str());
    std::remove(hufPath.c_str());
    std::remove(outPath.c_str());

    std::cout << "[PASS] " << testName << " (Restored " << decompStats.restoredFileSize
              << " bytes from " << decompStats.decodedBitCount << " bits)." << std::endl;
}

} // namespace

int main() {
    std::cout << "=== FileZipper: FileDecompressor & Round-Trip Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Empty file round-trip
    // -------------------------------------------------------------------------
    verifyRoundTrip("Test 1: Empty file compression & decompression round-trip", "");

    // -------------------------------------------------------------------------
    // Test 2: Single-character file round-trip
    // -------------------------------------------------------------------------
    verifyRoundTrip("Test 2: Single-character file ('A' x 500) round-trip", std::string(500, 'A'));

    // -------------------------------------------------------------------------
    // Test 3: Multi-character text file round-trip
    // -------------------------------------------------------------------------
    verifyRoundTrip(
        "Test 3: Multi-character text file round-trip",
        "ABRACADABRA! Huffman Coding is an optimal prefix-free lossless compression algorithm.\n"
    );

    // -------------------------------------------------------------------------
    // Test 4: Binary file round-trip including 0x00, 0xFF, and all 256 byte values
    // -------------------------------------------------------------------------
    {
        std::string allBytesData;
        allBytesData.reserve(256 * 8);
        for (int repeat = 0; repeat < 8; ++repeat) {
            for (int b = 0; b < 256; ++b) {
                allBytesData.push_back(static_cast<char>(static_cast<unsigned char>(b)));
            }
        }
        // Add extra null bytes (0x00) and 0xFF bytes to skew frequencies
        allBytesData.append(100, '\x00');
        allBytesData.append(75, '\xFF');

        verifyRoundTrip(
            "Test 4: Binary file round-trip (all 256 byte values + 0x00 and 0xFF)",
            allBytesData
        );
    }

    // -------------------------------------------------------------------------
    // Test 5: Large file round-trip (~100 KB)
    // -------------------------------------------------------------------------
    {
        std::string largeContent;
        largeContent.reserve(105000);
        for (int i = 0; i < 1500; ++i) {
            largeContent += "Line " + std::to_string(i) +
                            ": FileZipper C++17 Huffman Compression & Decompression Verification!\n";
        }
        verifyRoundTrip("Test 5: Large file (~105 KB) round-trip byte-for-byte match", largeContent);
    }

    // -------------------------------------------------------------------------
    // Test 6: Error handling — Invalid magic signature
    // -------------------------------------------------------------------------
    {
        const std::string badMagicPath = "temp_bad_magic.huf";
        const std::string outPath = "temp_bad_magic_out.bin";

        std::string fakeFile(FileCompressor::kHeaderSizeBytes, '\0');
        fakeFile[0] = 'B';
        fakeFile[1] = 'A';
        fakeFile[2] = 'D';
        fakeFile[3] = '!';
        writeFileBytes(badMagicPath, fakeFile);

        FileDecompressor decompressor;
        bool caughtBadMagic = false;
        try {
            decompressor.decompressFile(badMagicPath, outPath);
        } catch (const std::runtime_error&) {
            caughtBadMagic = true;
        }
        assert(caughtBadMagic == true);

        std::remove(badMagicPath.c_str());
        std::remove(outPath.c_str());
        std::cout << "[PASS] Test 6: Invalid magic signature rejected cleanly." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 7: Error handling — Truncated header & truncated bitstream
    // -------------------------------------------------------------------------
    {
        const std::string truncHeaderPath = "temp_trunc_header.huf";
        const std::string outPath = "temp_trunc_out.bin";

        // Write only 100 bytes (smaller than the 2068-byte .huf header)
        writeFileBytes(truncHeaderPath, "HUF1_truncated_header_data");

        FileDecompressor decompressor;
        bool caughtTruncHeader = false;
        try {
            decompressor.decompressFile(truncHeaderPath, outPath);
        } catch (const std::runtime_error&) {
            caughtTruncHeader = true;
        }
        assert(caughtTruncHeader == true);
        std::remove(truncHeaderPath.c_str());

        // Now compress a valid file, then truncate its payload bytes
        const std::string inPath = "temp_valid_in.txt";
        const std::string hufPath = "temp_trunc_bitstream.huf";
        writeFileBytes(inPath, "This file will have its compressed bitstream truncated!");

        FileCompressor compressor;
        compressor.compressFile(inPath, hufPath);

        // Keep only the 2068-byte header and strip the compressed bitstream payload
        std::string fullHuf = readFileBytes(hufPath);
        writeFileBytes(hufPath, fullHuf.substr(0, FileCompressor::kHeaderSizeBytes));

        bool caughtTruncBitstream = false;
        try {
            decompressor.decompressFile(hufPath, outPath);
        } catch (const std::runtime_error&) {
            caughtTruncBitstream = true;
        }
        assert(caughtTruncBitstream == true);

        std::remove(inPath.c_str());
        std::remove(hufPath.c_str());
        std::remove(outPath.c_str());
        std::cout << "[PASS] Test 7: Truncated header and truncated bitstream rejected cleanly." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 8: Error handling — Corrupted frequency table & invalid file paths
    // -------------------------------------------------------------------------
    {
        const std::string corruptPath = "temp_corrupt_freq.huf";
        const std::string outPath = "temp_corrupt_out.bin";

        // Create a header where originalFileSize = 50, but all frequencies are 0
        std::ofstream out(corruptPath, std::ios::binary | std::ios::trunc);
        FrequencyTable zeroFreqs{};
        FileCompressor::writeHeader(out, 50, zeroFreqs, 100);
        out.close();

        FileDecompressor decompressor;
        bool caughtCorruptFreq = false;
        try {
            decompressor.decompressFile(corruptPath, outPath);
        } catch (const std::runtime_error&) {
            caughtCorruptFreq = true;
        }
        assert(caughtCorruptFreq == true);
        std::remove(corruptPath.c_str());

        // Non-existent input path
        bool caughtMissingInput = false;
        try {
            decompressor.decompressFile("non_existent_archive_12345.huf", outPath);
        } catch (const std::runtime_error&) {
            caughtMissingInput = true;
        }
        assert(caughtMissingInput == true);

        std::remove(outPath.c_str());
        std::cout << "[PASS] Test 8: Corrupted frequency table and invalid file paths rejected cleanly." << std::endl;
    }

    std::cout << "All FileDecompressor unit tests passed successfully!" << std::endl;
    return 0;
}
