#include "huffman_node.h"

Node::Node(unsigned char ch, std::size_t freq, Node* l, Node* r)
    : character(ch), frequency(freq), left(l), right(r) {}

bool Node::isLeaf() const {
    return left == nullptr && right == nullptr;
}

bool CompareNode::operator()(const Node* lhs, const Node* rhs) const {
    if (lhs->frequency != rhs->frequency) {
        return lhs->frequency > rhs->frequency; // Min-heap: smaller frequency at top
    }
    return lhs->character > rhs->character;
}

void freeHuffmanTree(Node* root) {
    if (root == nullptr) {
        return;
    }
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    delete root;
}
