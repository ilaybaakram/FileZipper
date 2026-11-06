#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include "frequency_analyzer.h"
#include "huffman_node.h"

#include <cstddef>

/**
 * @brief Builds and manages the optimal Huffman Coding binary tree from a
 *        256-entry byte frequency table.
 *
 * Uses a greedy algorithm backed by a Min-Heap (std::priority_queue with
 * CompareNode) and owns the dynamically allocated Node hierarchy via RAII
 * (automatically calling freeHuffmanTree upon destruction or rebuild).
 */
class HuffmanTree {
public:
    HuffmanTree();
    ~HuffmanTree();

    // Disable copy semantics to prevent accidental double-free of Node pointers
    HuffmanTree(const HuffmanTree&) = delete;
    HuffmanTree& operator=(const HuffmanTree&) = delete;

    // Enable move semantics for safe transfer of tree ownership
    HuffmanTree(HuffmanTree&& other) noexcept;
    HuffmanTree& operator=(HuffmanTree&& other) noexcept;

    /**
     * @brief Builds the Huffman Tree from a 256-entry byte frequency table.
     *
     * Algorithm:
     * 1. Clears any existing tree nodes.
     * 2. Creates a leaf Node for every byte (0..255) with a non-zero frequency
     *    and pushes it into a Min-Heap ordered by CompareNode.
     * 3. If the heap is empty (empty file), root remains nullptr.
     * 4. If only one unique byte exists, that single leaf node becomes the root.
     * 5. Otherwise, repeatedly extracts the two lowest-frequency nodes, merges
     *    them under a new internal parent node with their combined frequency,
     *    and inserts the parent back into the Min-Heap until one root remains.
     *
     * @param frequencies The 256-entry byte frequency table.
     */
    void buildFromFrequencies(const FrequencyTable& frequencies);

    /**
     * @brief Returns a pointer to the root node of the Huffman Tree (or nullptr if empty).
     */
    const Node* getRoot() const;

    /**
     * @brief Checks whether the tree is empty (root == nullptr).
     */
    bool isEmpty() const;

    /**
     * @brief Returns the total frequency stored at the root of the tree (0 if empty).
     */
    std::size_t getTotalFrequency() const;

    /**
     * @brief Counts and returns the number of leaf nodes currently in the tree.
     */
    std::size_t getLeafCount() const;

    /**
     * @brief Deallocates all nodes in the tree and resets root to nullptr.
     */
    void clear();

private:
    Node* root_;

    static std::size_t countLeavesRecursive(const Node* node);
};

#endif // HUFFMAN_TREE_H
