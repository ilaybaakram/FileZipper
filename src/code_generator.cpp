#include "code_generator.h"

CodeGenerator::CodeGenerator() : codes_{} {}

void CodeGenerator::reset() {
    codes_.fill("");
}

void CodeGenerator::generateCodes(const Node* root) {
    reset();

    if (root == nullptr) {
        return;
    }

    // Edge case: Tree has only a single node (input had only 1 unique byte).
    // Assign "0" because an empty path "" would produce 0 bits when encoding.
    if (root->isLeaf()) {
        codes_[root->character] = "0";
        return;
    }

    traverseRecursive(root, "");
}

const CodeTable& CodeGenerator::getCodeTable() const {
    return codes_;
}

const std::string& CodeGenerator::getCode(unsigned char byte) const {
    return codes_[byte];
}

bool CodeGenerator::hasCode(unsigned char byte) const {
    return !codes_[byte].empty();
}

std::size_t CodeGenerator::getEncodedSymbolCount() const {
    std::size_t count = 0;
    for (const std::string& code : codes_) {
        if (!code.empty()) {
            ++count;
        }
    }
    return count;
}

std::uint64_t CodeGenerator::calculateTotalEncodedBits(const FrequencyTable& frequencies) const {
    std::uint64_t totalBits = 0;
    for (std::size_t i = 0; i < codes_.size(); ++i) {
        if (!codes_[i].empty() && frequencies[i] > 0) {
            totalBits += frequencies[i] * static_cast<std::uint64_t>(codes_[i].size());
        }
    }
    return totalBits;
}

bool CodeGenerator::isPrefixFree() const {
    for (std::size_t i = 0; i < codes_.size(); ++i) {
        if (codes_[i].empty()) {
            continue;
        }
        for (std::size_t j = 0; j < codes_.size(); ++j) {
            if (i == j || codes_[j].empty()) {
                continue;
            }
            // Check if codes_[i] is a prefix of codes_[j]
            if (codes_[j].size() >= codes_[i].size() &&
                codes_[j].compare(0, codes_[i].size(), codes_[i]) == 0) {
                return false;
            }
        }
    }
    return true;
}

void CodeGenerator::traverseRecursive(const Node* node, const std::string& currentCode) {
    if (node == nullptr) {
        return;
    }

    if (node->isLeaf()) {
        codes_[node->character] = currentCode;
        return;
    }

    traverseRecursive(node->left, currentCode + "0");
    traverseRecursive(node->right, currentCode + "1");
}
