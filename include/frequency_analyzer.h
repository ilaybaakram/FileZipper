#ifndef FREQUENCY_ANALYZER_H
#define FREQUENCY_ANALYZER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>

/**
 * @brief Fixed-size frequency table mapping every possible byte value (0..255)
 *        to its 64-bit occurrence count.
 */
using FrequencyTable = std::array<std::uint64_t, 256>;

/**
 * @brief Analyzes input data (files, binary streams, or in-memory buffers)
 *        to count the frequency of all 256 possible byte values (0x00 - 0xFF).
 *
 * All file operations are performed in binary mode so both text and arbitrary
 * binary files (including null bytes and non-ASCII bytes) are handled accurately.
 */
class FrequencyAnalyzer {
public:
    /**
     * @brief Constructs an analyzer with all 256 byte counts initialized to 0.
     */
    FrequencyAnalyzer();

    /**
     * @brief Resets all frequency counts and total byte count back to 0.
     */
    void reset();

    /**
     * @brief Reads a file in binary mode and counts the frequency of every byte.
     *
     * Resets any previous state before analyzing the file.
     *
     * @param filePath Path to the file on disk.
     * @throws std::runtime_error if the file cannot be opened or a read error occurs.
     */
    void analyzeFile(const std::string& filePath);

    /**
     * @brief Reads all bytes from an input stream and counts their frequencies.
     *
     * Resets any previous state before analyzing the stream.
     *
     * @param stream Input stream (should be opened with std::ios::binary).
     */
    void analyzeStream(std::istream& stream);

    /**
     * @brief Counts byte frequencies from an in-memory string or byte buffer.
     *
     * Resets any previous state before analyzing the buffer.
     *
     * @param data Input byte sequence.
     */
    void analyzeBuffer(const std::string& data);

    /**
     * @brief Returns a const reference to the 256-entry frequency table.
     */
    const FrequencyTable& getFrequencies() const;

    /**
     * @brief Returns the frequency count for a specific byte value (0..255).
     */
    std::uint64_t getFrequency(unsigned char byte) const;

    /**
     * @brief Returns the total number of bytes processed in the last analysis.
     */
    std::uint64_t getTotalBytes() const;

    /**
     * @brief Returns the number of distinct byte values that had a non-zero frequency.
     */
    std::size_t getUniqueByteCount() const;

private:
    FrequencyTable frequencies_;
    std::uint64_t totalBytes_;
};

#endif // FREQUENCY_ANALYZER_H
