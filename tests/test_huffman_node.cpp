#include "huffman_node.h"

#include <cassert>
#include <iostream>
#include <queue>
#include <vector>

int main() {
    std::cout << "=== FileZipper: Huffman Node Unit Tests ===" << std::endl;

    // Test 1: Leaf node construction and properties
    Node* leafA = new Node('A', 5);
    assert(leafA->character == 'A');
    assert(leafA->frequency == 5);
    assert(leafA->left == nullptr);
    assert(leafA->right == nullptr);
    assert(leafA->isLeaf() == true);
    std::cout << "[PASS] Test 1: Leaf node ('A', freq=5) created and verified as leaf." << std::endl;

    // Test 2: Second leaf node construction
    Node* leafB = new Node('B', 9);
    assert(leafB->character == 'B');
    assert(leafB->frequency == 9);
    assert(leafB->isLeaf() == true);
    std::cout << "[PASS] Test 2: Leaf node ('B', freq=9) created and verified as leaf." << std::endl;

    // Test 3: Internal node construction linking two children
    Node* internalNode = new Node('\0', leafA->frequency + leafB->frequency, leafA, leafB);
    assert(internalNode->frequency == 14);
    assert(internalNode->left == leafA);
    assert(internalNode->right == leafB);
    assert(internalNode->isLeaf() == false);
    std::cout << "[PASS] Test 3: Internal node (freq=14) linked to children and verified as non-leaf." << std::endl;

    // Test 4: Single-child node is not a leaf
    Node* singleChildLeaf = new Node('C', 3);
    Node* parentWithOneChild = new Node('\0', 3, singleChildLeaf, nullptr);
    assert(parentWithOneChild->isLeaf() == false);
    assert(singleChildLeaf->isLeaf() == true);
    std::cout << "[PASS] Test 4: Single-child node correctly reports isLeaf() == false." << std::endl;

    // Test 5: CompareNode ordering in std::priority_queue (min-heap behavior)
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> minHeap;
    minHeap.push(leafB);           // freq 9
    minHeap.push(internalNode);    // freq 14
    minHeap.push(singleChildLeaf); // freq 3
    minHeap.push(leafA);           // freq 5

    assert(minHeap.top() == singleChildLeaf);
    minHeap.pop();
    assert(minHeap.top() == leafA);
    minHeap.pop();
    assert(minHeap.top() == leafB);
    minHeap.pop();
    assert(minHeap.top() == internalNode);
    minHeap.pop();
    std::cout << "[PASS] Test 5: CompareNode min-heap priority queue ordering verified." << std::endl;

    // Clean up allocated memory
    freeHuffmanTree(internalNode); // Frees internalNode, leafA, and leafB
    freeHuffmanTree(parentWithOneChild); // Frees parentWithOneChild and singleChildLeaf
    std::cout << "[PASS] Test 6: Huffman tree memory deallocated cleanly." << std::endl;

    std::cout << "All Huffman Node tests passed successfully!" << std::endl;
    return 0;
}
