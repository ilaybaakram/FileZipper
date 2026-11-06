#ifndef BIT_WRITER_H
#define BIT_WRITER_H

#include <cstdint>
#include <ostream>
#include <string>

/**
 * @brief Buffers individual bits ('0' and '1') into an 8-bit byte accumulator
 *        and writes packed bytes to a binary output stream.
 *
 * Bits are packed from Most Significant Bit (MSB, bit 7) down to Least
 * Significant Bit (LSB, bit 0). Whenever 8 bits are accumulated, the full byte
 * is written to the underlying std::ostream.
 *
 * Calling flush() pads any remaining partial byte (1..7 bits) with trailing
 * zero-bits in the lower positions and writes the final byte.
 */
class BitWriter {
public:
    /**
     * @brief Constructs a BitWriter bound to a binary output stream.
     *
     * @param stream Output stream (must be opened with std::ios::binary).
     */
    explicit BitWriter(std::ostream& stream);

    /**
     * @brief Writes a single bit (0 or 1) into the current byte buffer.
     *
     * @param bit Non-zero writes a 1-bit; zero writes a 0-bit.
     */
    void writeBit(int bit);

    /**
     * @brief Writes a sequence of bits from a string of '0' and '1' characters.
     *
     * @param bitString String containing only '0' and '1' characters (e.g., "1101").
     * @throws std::invalid_argument if any character is neither '0' nor '1'.
     */
    void writeBits(const std::string& bitString);

    /**
     * @brief Pads any remaining bits (1..7) in the current byte buffer with
     *        trailing zeros and writes the final byte to the output stream.
     *        If the buffer is already empty (0 pending bits), does nothing.
     *
     * @return Number of zero-padding bits added (0..7).
     */
    std::uint8_t flush();

    /**
     * @brief Returns the total number of meaningful (unpadded) bits written so far.
     */
    std::uint64_t getMeaningfulBitsWritten() const;

    /**
     * @brief Returns the total number of bytes emitted to the output stream so far.
     */
    std::uint64_t getBytesWritten() const;

    /**
     * @brief Returns the number of bits currently buffered in the partial byte (0..7).
     */
    std::uint8_t getPendingBitCount() const;

private:
    std::ostream& stream_;
    std::uint8_t currentByte_;
    std::uint8_t bitCount_;
    std::uint64_t meaningfulBitsWritten_;
    std::uint64_t bytesWritten_;
};

#endif // BIT_WRITER_H
