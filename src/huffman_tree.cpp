#include "huffman_tree.h"

#include <queue>
#include <vector>

HuffmanTree::HuffmanTree() : root_(nullptr) {}

HuffmanTree::~HuffmanTree() {
    clear();
}

HuffmanTree::HuffmanTree(HuffmanTree&& other) noexcept : root_(other.root_) {
    other.root_ = nullptr;
}

HuffmanTree& HuffmanTree::operator=(HuffmanTree&& other) noexcept {
    if (this != &other) {
        clear();
        root_ = other.root_;
        other.root_ = nullptr;
    }
    return *this;
}

void HuffmanTree::buildFromFrequencies(const FrequencyTable& frequencies) {
    clear();

    // Min-Heap of Node pointers ordered by frequency (and character on ties)
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> minHeap;

    // Step 1: Create leaf nodes for all bytes with non-zero frequency
    for (std::size_t byteValue = 0; byteValue < frequencies.size(); ++byteValue) {
        if (frequencies[byteValue] > 0) {
            unsigned char ch = static_cast<unsigned char>(byteValue);
            std::size_t freq = static_cast<std::size_t>(frequencies[byteValue]);
            minHeap.push(new Node(ch, freq));
        }
    }

    // Step 2: Handle empty input (all frequencies were zero)
    if (minHeap.empty()) {
        return;
    }

    // Step 3: Greedy merging loop — combine the two lowest-frequency nodes
    // Note: If minHeap.size() == 1 (file with a single unique byte), this loop
    // does not execute and that single leaf node becomes root_.
    while (minHeap.size() > 1) {
        Node* leftChild = minHeap.top();
        minHeap.pop();

        Node* rightChild = minHeap.top();
        minHeap.pop();

        // Store the smaller character of the two children in the internal node
        // so CompareNode always breaks frequency ties deterministically.
        unsigned char tieBreakerChar = (leftChild->character < rightChild->character)
                                           ? leftChild->character
                                           : rightChild->character;

        std::size_t combinedFrequency = leftChild->frequency + rightChild->frequency;

        Node* parent = new Node(tieBreakerChar, combinedFrequency, leftChild, rightChild);
        minHeap.push(parent);
    }

    // Step 4: The last remaining node in the Min-Heap is the Huffman Tree root
    root_ = minHeap.top();
}

const Node* HuffmanTree::getRoot() const {
    return root_;
}

bool HuffmanTree::isEmpty() const {
    return root_ == nullptr;
}

std::size_t HuffmanTree::getTotalFrequency() const {
    return root_ != nullptr ? root_->frequency : 0;
}

std::size_t HuffmanTree::getLeafCount() const {
    return countLeavesRecursive(root_);
}

void HuffmanTree::clear() {
    freeHuffmanTree(root_);
    root_ = nullptr;
}

std::size_t HuffmanTree::countLeavesRecursive(const Node* node) {
    if (node == nullptr) {
        return 0;
    }
    if (node->isLeaf()) {
        return 1;
    }
    return countLeavesRecursive(node->left) + countLeavesRecursive(node->right);
}
