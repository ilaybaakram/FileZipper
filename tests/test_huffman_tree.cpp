#include "frequency_analyzer.h"
#include "huffman_node.h"
#include "huffman_tree.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

namespace {

// Helper function to write raw bytes (including null bytes) to a temporary file
void writeTempFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    assert(out.is_open());
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.close();
}

// Recursively verifies that every internal node's frequency equals the sum of its
// children's frequencies, and every leaf node's frequency matches the table.
void verifyTreeInvariants(const Node* node, const FrequencyTable& expectedFreqs, std::size_t& leafFreqSum) {
    assert(node != nullptr);

    if (node->isLeaf()) {
        assert(node->left == nullptr);
        assert(node->right == nullptr);
        assert(node->frequency == expectedFreqs[node->character]);
        leafFreqSum += node->frequency;
        return;
    }

    // Internal node must have two valid children whose frequencies sum to parent's frequency
    assert(node->left != nullptr);
    assert(node->right != nullptr);
    assert(node->frequency == node->left->frequency + node->right->frequency);

    verifyTreeInvariants(node->left, expectedFreqs, leafFreqSum);
    verifyTreeInvariants(node->right, expectedFreqs, leafFreqSum);
}

} // namespace

int main() {
    std::cout << "=== FileZipper: Frequency Analyzer & Huffman Tree Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Frequency analysis of a simple text file & total frequency count
    // -------------------------------------------------------------------------
    {
        const std::string tempPath = "temp_test_simple.txt";
        const std::string content = "Hello World!\n";
        writeTempFile(tempPath, content);

        FrequencyAnalyzer analyzer;
        analyzer.analyzeFile(tempPath);

        assert(analyzer.getTotalBytes() == content.size());
        assert(analyzer.getFrequency('l') == 3);
        assert(analyzer.getFrequency('o') == 2);
        assert(analyzer.getFrequency('H') == 1);
        assert(analyzer.getFrequency(' ') == 1);
        assert(analyzer.getFrequency('\n') == 1);
        assert(analyzer.getFrequency('z') == 0);

        std::uint64_t sumOfTable = 0;
        for (std::uint64_t count : analyzer.getFrequencies()) {
            sumOfTable += count;
        }
        assert(sumOfTable == analyzer.getTotalBytes());

        std::remove(tempPath.c_str());
        std::cout << "[PASS] Test 1: Simple text file frequency analysis and total byte count verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Frequency analysis of repeated characters
    // -------------------------------------------------------------------------
    {
        const std::string tempPath = "temp_test_repeated.txt";
        const std::string content(1024, 'Z');
        writeTempFile(tempPath, content);

        FrequencyAnalyzer analyzer;
        analyzer.analyzeFile(tempPath);

        assert(analyzer.getTotalBytes() == 1024);
        assert(analyzer.getUniqueByteCount() == 1);
        assert(analyzer.getFrequency('Z') == 1024);
        assert(analyzer.getFrequency('A') == 0);

        std::remove(tempPath.c_str());
        std::cout << "[PASS] Test 2: Repeated character frequency analysis (1024 'Z's) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Frequency analysis of binary data containing null bytes (0x00) and 0xFF
    // -------------------------------------------------------------------------
    {
        const std::string tempPath = "temp_test_binary.bin";
        const std::string binaryData({'\x00', '\xFF', '\x00', '\x80', '\x7F', '\x00', '\xFF', '\x01'});
        writeTempFile(tempPath, binaryData);

        FrequencyAnalyzer analyzer;
        analyzer.analyzeFile(tempPath);

        assert(analyzer.getTotalBytes() == 8);
        assert(analyzer.getUniqueByteCount() == 5);
        assert(analyzer.getFrequency(0x00) == 3);
        assert(analyzer.getFrequency(0xFF) == 2);
        assert(analyzer.getFrequency(0x80) == 1);
        assert(analyzer.getFrequency(0x7F) == 1);
        assert(analyzer.getFrequency(0x01) == 1);

        std::remove(tempPath.c_str());
        std::cout << "[PASS] Test 3: Binary data with null bytes (0x00) and high bytes (0x80, 0xFF) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Empty file handling
    // -------------------------------------------------------------------------
    {
        const std::string tempPath = "temp_test_empty.txt";
        writeTempFile(tempPath, "");

        FrequencyAnalyzer analyzer;
        analyzer.analyzeFile(tempPath);

        assert(analyzer.getTotalBytes() == 0);
        assert(analyzer.getUniqueByteCount() == 0);
        for (std::uint64_t count : analyzer.getFrequencies()) {
            assert(count == 0);
        }

        std::remove(tempPath.c_str());
        std::cout << "[PASS] Test 4: Empty file handling verified (0 total bytes, all counts 0)." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 5: Empty frequency table -> Huffman Tree handling
    // -------------------------------------------------------------------------
    {
        FrequencyTable emptyTable{};
        HuffmanTree tree;
        tree.buildFromFrequencies(emptyTable);

        assert(tree.isEmpty() == true);
        assert(tree.getRoot() == nullptr);
        assert(tree.getTotalFrequency() == 0);
        assert(tree.getLeafCount() == 0);
        std::cout << "[PASS] Test 5: Empty frequency table builds an empty Huffman Tree safely." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 6: Single-symbol input -> Huffman Tree handling
    // -------------------------------------------------------------------------
    {
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer("AAAAAAA");

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        assert(tree.isEmpty() == false);
        assert(tree.getTotalFrequency() == 7);
        assert(tree.getLeafCount() == 1);

        const Node* root = tree.getRoot();
        assert(root != nullptr);
        assert(root->isLeaf() == true);
        assert(root->character == 'A');
        assert(root->frequency == 7);
        std::cout << "[PASS] Test 6: Single-symbol input ('AAAAAAA') builds single-node leaf root." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 7: Huffman Tree construction with several byte values, parent frequency
    //         correctness, and leaf count verification
    // -------------------------------------------------------------------------
    {
        // "ABRACADABRA": A=5, B=2, R=2, C=1, D=1 (Total = 11, Unique leaves = 5)
        FrequencyAnalyzer analyzer;
        analyzer.analyzeBuffer("ABRACADABRA");

        HuffmanTree tree;
        tree.buildFromFrequencies(analyzer.getFrequencies());

        assert(!tree.isEmpty());
        assert(tree.getTotalFrequency() == 11);
        assert(tree.getLeafCount() == 5);

        const Node* root = tree.getRoot();
        assert(root != nullptr);
        assert(!root->isLeaf());
        assert(root->frequency == 11);

        // Verify greedy merges:
        // Step 1: C(1) + D(1) -> CD(2)
        // Step 2: B(2) + CD(2) -> BCD(4) (since 'B' < 'C' < 'R')
        // Step 3: R(2) + BCD(4) -> RBCD(6)
        // Step 4: A(5) + RBCD(6) -> Root(11)
        assert(root->left->isLeaf() && root->left->character == 'A' && root->left->frequency == 5);
        assert(!root->right->isLeaf() && root->right->frequency == 6);

        std::size_t leafFreqSum = 0;
        verifyTreeInvariants(root, analyzer.getFrequencies(), leafFreqSum);
        assert(leafFreqSum == analyzer.getTotalBytes());

        std::cout << "[PASS] Test 7: Multi-symbol Huffman Tree construction, parent frequencies, and leaf count verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 8: Min-Heap ordering and deterministic tie-breaking
    // -------------------------------------------------------------------------
    {
        std::priority_queue<Node*, std::vector<Node*>, CompareNode> minHeap;
        Node* n1 = new Node('D', 4);
        Node* n2 = new Node('B', 2);
        Node* n3 = new Node('A', 2); // Same frequency as 'B', smaller ASCII
        Node* n4 = new Node('C', 1);

        minHeap.push(n1);
        minHeap.push(n2);
        minHeap.push(n3);
        minHeap.push(n4);

        // Expected extraction order: C(1), A(2), B(2), D(4)
        assert(minHeap.top() == n4);
        minHeap.pop();
        assert(minHeap.top() == n3);
        minHeap.pop();
        assert(minHeap.top() == n2);
        minHeap.pop();
        assert(minHeap.top() == n1);
        minHeap.pop();

        delete n1;
        delete n2;
        delete n3;
        delete n4;

        std::cout << "[PASS] Test 8: Min-Heap priority ordering and deterministic tie-breaking verified." << std::endl;
    }

    std::cout << "All Frequency Analyzer and Huffman Tree tests passed successfully!" << std::endl;
    return 0;
}
