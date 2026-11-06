#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "frequency_analyzer.h"
#include "huffman_node.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

/**
 * @brief Fixed-size lookup table mapping every possible byte value (0..255)
 *        to its variable-length Huffman binary code string (e.g., "0", "101").
 *        Bytes that do not appear in the input have an empty string "".
 */
using CodeTable = std::array<std::string, 256>;

/**
 * @brief Traverses a constructed Huffman Tree to generate prefix-free binary
 *        codes for each unique byte, and computes compression bit metrics.
 *
 * Traversal Convention:
 * - Moving to the left child appends '0'
 * - Moving to the right child appends '1'
 * - Special Case (Single Unique Byte): If the root itself is a leaf node,
 *   it is assigned the 1-bit code "0" so that every occurrence can be
 *   encoded using at least one bit.
 */
class CodeGenerator {
public:
    CodeGenerator();

    /**
     * @brief Clears all generated codes back to empty strings.
     */
    void reset();

    /**
     * @brief Generates the 256-entry Huffman prefix code table from a tree root.
     *
     * @param root Pointer to the root node of the Huffman Tree (may be nullptr).
     */
    void generateCodes(const Node* root);

    /**
     * @brief Returns a const reference to the full 256-entry code table.
     */
    const CodeTable& getCodeTable() const;

    /**
     * @brief Returns the binary code string ("0"/"1" sequence) for a given byte.
     *        Returns "" if the byte was not present in the Huffman Tree.
     */
    const std::string& getCode(unsigned char byte) const;

    /**
     * @brief Checks whether a non-empty Huffman code exists for the given byte.
     */
    bool hasCode(unsigned char byte) const;

    /**
     * @brief Returns the number of distinct bytes that have been assigned a code.
     */
    std::size_t getEncodedSymbolCount() const;

    /**
     * @brief Computes the total number of bits required to encode the payload
     *        using the generated Huffman codes and the given byte frequencies:
     *        Sum over all bytes b of (frequencies[b] * codeTable[b].length()).
     *
     * @param frequencies The 256-entry frequency table of the input data.
     * @return Total compressed payload size in bits.
     */
    std::uint64_t calculateTotalEncodedBits(const FrequencyTable& frequencies) const;

    /**
     * @brief Verifies the Prefix-Free Property across all generated codes:
     *        no valid code in the table is a prefix of any other valid code.
     *
     * @return true if the code table is strictly prefix-free; false otherwise.
     */
    bool isPrefixFree() const;

private:
    CodeTable codes_;

    void traverseRecursive(const Node* node, const std::string& currentCode);
};

#endif // CODE_GENERATOR_H
