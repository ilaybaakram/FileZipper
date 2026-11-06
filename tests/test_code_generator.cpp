#include "code_generator.h"
#include "frequency_analyzer.h"
#include "huffman_tree.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== FileZipper: Huffman Prefix Code Generator Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Empty tree produces empty code table
    // -------------------------------------------------------------------------
    {
        CodeGenerator generator;
        generator.generateCodes(nullptr);

        assert(generator.getEncodedSymbolCount() == 0);
        assert(generator.isPrefixFree() == true);
        for (std::size_t i = 0; i < 256; ++i) {
            assert(!generator.hasCode(static_cast<unsigned char>(i)));
            assert(generator.getCode(static_cast<unsigned char>(i)).empty());
        }
        std::cout << "[PASS] Test 1: Empty tree produces an empty code table." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Single-symbol input ("AAAAAAA") assigns 1-bit code "0"
    // -------------------------------------------------------------------------
    {
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer("AAAAAAA");

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        CodeGenerator generator;
        generator.generateCodes(tree.getRoot());

        assert(generator.getEncodedSymbolCount() == 1);
        assert(generator.hasCode('A') == true);
        assert(generator.getCode('A') == "0");
        assert(generator.hasCode('B') == false);
        assert(generator.isPrefixFree() == true);
        assert(generator.calculateTotalEncodedBits(analyzer.getFrequencies()) == 7);

        std::cout << "[PASS] Test 2: Single-symbol input ('AAAAAAA') assigned code \"0\" (7 total bits)." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Multi-symbol input ("ABRACADABRA") code generation, optimality,
    //         and prefix-free verification
    // -------------------------------------------------------------------------
    {
        // "ABRACADABRA": A=5, B=2, R=2, C=1, D=1 (11 bytes = 88 uncompressed bits)
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer("ABRACADABRA");

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        CodeGenerator generator;
        generator.generateCodes(tree.getRoot());

        assert(generator.getEncodedSymbolCount() == 5);
        assert(generator.isPrefixFree() == true);

        // Verify all 5 unique characters received non-empty binary strings of '0' and '1'
        const std::string chars = "ABRCD";
        for (char c : chars) {
            unsigned char byte = static_cast<unsigned char>(c);
            assert(generator.hasCode(byte));
            const std::string& code = generator.getCode(byte);
            assert(!code.empty());
            for (char bit : code) {
                assert(bit == '0' || bit == '1');
            }
        }

        // Verify Huffman optimality: more frequent characters have shorter or equal code lengths
        // Frequencies: A(5) > B(2) == R(2) > C(1) == D(1)
        assert(generator.getCode('A').size() == 1); // 'A' is direct child of root -> 1 bit ("0")
        assert(generator.getCode('A').size() <= generator.getCode('B').size());
        assert(generator.getCode('A').size() <= generator.getCode('R').size());
        assert(generator.getCode('B').size() <= generator.getCode('C').size());
        assert(generator.getCode('B').size() <= generator.getCode('D').size());

        // Exact deterministic codes for "ABRACADABRA":
        // A(5) -> "0" (len 1)
        // R(2) -> "10" (len 2)
        // B(2) -> "110" (len 3)
        // C(1) -> "1110" (len 4)
        // D(1) -> "1111" (len 4)
        // Total bits = 5*1 + 2*2 + 2*3 + 1*4 + 1*4 = 5 + 4 + 6 + 4 + 4 = 23 bits (vs 88 raw bits!)
        assert(generator.getCode('A') == "0");
        assert(generator.getCode('R') == "10");
        assert(generator.getCode('B') == "110");
        assert(generator.getCode('C') == "1110");
        assert(generator.getCode('D') == "1111");

        std::uint64_t encodedBits = generator.calculateTotalEncodedBits(analyzer.getFrequencies());
        assert(encodedBits == 23);
        assert(encodedBits < analyzer.getTotalBytes() * 8);

        std::cout << "[PASS] Test 3: Multi-symbol ('ABRACADABRA') codes, prefix-free property, and bit savings (23 vs 88 bits) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Binary data with null byte (0x00) and 0xFF
    // -------------------------------------------------------------------------
    {
        const std::string binaryData({'\x00', '\xFF', '\x00', '\x80', '\x7F', '\x00', '\xFF', '\x01'});
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer(binaryData);

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        CodeGenerator generator;
        generator.generateCodes(tree.getRoot());

        assert(generator.getEncodedSymbolCount() == 5);
        assert(generator.isPrefixFree() == true);
        assert(generator.hasCode(0x00));
        assert(generator.hasCode(0xFF));
        assert(generator.hasCode(0x80));
        assert(generator.hasCode(0x7F));
        assert(generator.hasCode(0x01));

        // Most frequent byte (0x00, freq=3) must have the shortest code length
        assert(generator.getCode(0x00).size() <= generator.getCode(0x80).size());
        assert(generator.getCode(0x00).size() <= generator.getCode(0x01).size());

        std::cout << "[PASS] Test 4: Binary data (0x00, 0x7F, 0x80, 0xFF) prefix codes verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 5: Round-trip tree walk verification (every generated code leads back
    //         to its exact leaf character when walking from root)
    // -------------------------------------------------------------------------
    {
        const std::string sample = "Huffman Coding is a lossless data compression algorithm!";
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer(sample);

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        CodeGenerator generator;
        generator.generateCodes(tree.getRoot());

        assert(generator.isPrefixFree() == true);

        for (std::size_t b = 0; b < 256; ++b) {
            unsigned char byte = static_cast<unsigned char>(b);
            if (!generator.hasCode(byte)) {
                continue;
            }
            const std::string& code = generator.getCode(byte);
            const Node* curr = tree.getRoot();
            for (char bit : code) {
                assert(curr != nullptr && !curr->isLeaf());
                curr = (bit == '0') ? curr->left : curr->right;
            }
            assert(curr != nullptr && curr->isLeaf());
            assert(curr->character == byte);
        }

        std::cout << "[PASS] Test 5: Tree-walk verification confirmed every code maps back to its exact leaf byte." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 6: Deterministic code generation across repeated runs
    // -------------------------------------------------------------------------
    {
        const std::string sample = "Deterministic Huffman Code Generation Check 12345!";
        FrequencyAnalyzer analyzer1;
        analyzer1.analyzeBuffer(sample);
        HuffmanTree tree1;
        tree1.buildFromFrequencies(analyzer1.getFrequencies());
        CodeGenerator gen1;
        gen1.generateCodes(tree1.getRoot());

        FrequencyAnalyzer analyzer2;
        analyzer2.analyzeBuffer(sample);
        HuffmanTree tree2;
        tree2.buildFromFrequencies(analyzer2.getFrequencies());
        CodeGenerator gen2;
        gen2.generateCodes(tree2.getRoot());

        assert(gen1.getCodeTable() == gen2.getCodeTable());
        std::cout << "[PASS] Test 6: Deterministic code generation across multiple runs verified." << std::endl;
    }

    std::cout << "All Huffman Prefix Code Generator tests passed successfully!" << std::endl;
    return 0;
}
