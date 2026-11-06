#ifndef BIT_READER_H
#define BIT_READER_H

#include <cstdint>
#include <istream>
#include <limits>

/**
 * @brief Reads individual bits (MSB-first, bit 7 down to bit 0) from a binary
 *        input stream, matching the bit-packing layout produced by BitWriter.
 *
 * Tracks the number of meaningful bits read and enforces an optional maximum
 * meaningful bit limit so trailing zero-padding bits in the final byte are
 * never consumed as data.
 */
class BitReader {
public:
    /**
     * @brief Constructs a BitReader bound to a binary input stream.
     *
     * @param stream         Binary input stream (must be opened with std::ios::binary).
     * @param maxBitsToRead  Maximum number of meaningful bits allowed to be read
     *                       (defaults to unlimited if not specified).
     */
    explicit BitReader(
        std::istream& stream,
        std::uint64_t maxBitsToRead = std::numeric_limits<std::uint64_t>::max()
    );

    /**
     * @brief Reads the next single bit (0 or 1) from the input stream.
     *
     * Returns false without reading further if the configured meaningful bit
     * limit (maxBitsToRead) has already been reached or if the underlying
     * stream reaches End-Of-File before a bit can be extracted.
     *
     * @param bitOut Receives 0 or 1 when a bit is successfully read.
     * @return true if a valid bit was read; false if limit reached or EOF.
     * @throws std::runtime_error if a hardware/stream I/O error (badbit) occurs.
     */
    bool readBit(int& bitOut);

    /**
     * @brief Returns the number of meaningful bits read so far.
     */
    std::uint64_t getBitsRead() const;

    /**
     * @brief Returns the configured maximum number of meaningful bits to read.
     */
    std::uint64_t getMaxBitsToRead() const;

    /**
     * @brief Returns true if getBitsRead() < getMaxBitsToRead().
     */
    bool hasMoreMeaningfulBits() const;

    /**
     * @brief Returns the number of unread bits remaining in the current buffered byte (0..7).
     */
    std::uint8_t getBufferedBitCount() const;

private:
    std::istream& stream_;
    std::uint64_t maxBitsToRead_;
    std::uint64_t bitsRead_;
    std::uint8_t currentByte_;
    std::uint8_t bitsRemainingInByte_;
};

#endif // BIT_READER_H
