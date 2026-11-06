#ifndef HUFFMAN_NODE_H
#define HUFFMAN_NODE_H

#include <cstddef>

/**
 * @brief Represents a single node in the Huffman Coding binary tree.
 *
 * Leaf nodes store an actual byte/character and its frequency of occurrence
 * in the input stream. Internal nodes store the combined frequency of their
 * children and have non-null left and right child pointers.
 */
struct Node {
    unsigned char character;  ///< The byte/character represented (valid for leaf nodes)
    std::size_t frequency;    ///< Frequency count of the character or subtree
    Node* left;               ///< Pointer to the left child (0-bit branch)
    Node* right;              ///< Pointer to the right child (1-bit branch)

    /**
     * @brief Constructs a Huffman Tree Node.
     *
     * @param ch   The character/byte value (default: '\0' for internal nodes).
     * @param freq The frequency count of the node.
     * @param l    Pointer to the left child node (default: nullptr).
     * @param r    Pointer to the right child node (default: nullptr).
     */
    Node(unsigned char ch, std::size_t freq, Node* l = nullptr, Node* r = nullptr);

    /**
     * @brief Checks whether this node is a leaf node in the Huffman Tree.
     *
     * @return true if both left and right child pointers are nullptr; false otherwise.
     */
    bool isLeaf() const;
};

/**
 * @brief Functor used to compare two Node pointers in a min-priority queue.
 *
 * Orders nodes such that the node with the smaller frequency has higher priority.
 * When frequencies are equal, ties are broken deterministically by character value.
 */
struct CompareNode {
    bool operator()(const Node* lhs, const Node* rhs) const;
};

/**
 * @brief Recursively deallocates an entire Huffman Tree rooted at root.
 *
 * @param root Pointer to the root of the tree/subtree to free.
 */
void freeHuffmanTree(Node* root);

#endif // HUFFMAN_NODE_H
