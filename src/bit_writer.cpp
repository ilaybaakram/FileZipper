#include "bit_writer.h"

#include <stdexcept>

BitWriter::BitWriter(std::ostream& stream)
    : stream_(stream),
      currentByte_(0),
      bitCount_(0),
      meaningfulBitsWritten_(0),
      bytesWritten_(0) {}

void BitWriter::writeBit(int bit) {
    // Shift existing bits left by 1 and insert the new bit at the LSB (packs MSB-first)
    currentByte_ = static_cast<std::uint8_t>((currentByte_ << 1) | (bit ? 1 : 0));
    ++bitCount_;
    ++meaningfulBitsWritten_;

    // Once 8 bits have been accumulated, emit one full byte to the stream
    if (bitCount_ == 8) {
        char outByte = static_cast<char>(currentByte_);
        stream_.write(&outByte, 1);
        if (!stream_.good()) {
            throw std::runtime_error("Failed to write packed byte to output stream");
        }
        ++bytesWritten_;
        currentByte_ = 0;
        bitCount_ = 0;
    }
}

void BitWriter::writeBits(const std::string& bitString) {
    for (char ch : bitString) {
        if (ch == '0') {
            writeBit(0);
        } else if (ch == '1') {
            writeBit(1);
        } else {
            throw std::invalid_argument("BitWriter::writeBits expected only '0' or '1' characters");
        }
    }
}

std::uint8_t BitWriter::flush() {
    if (bitCount_ == 0) {
        return 0;
    }

    // Pad the remaining lower bit positions with 0s
    std::uint8_t paddingBits = static_cast<std::uint8_t>(8 - bitCount_);
    currentByte_ = static_cast<std::uint8_t>(currentByte_ << paddingBits);

    char outByte = static_cast<char>(currentByte_);
    stream_.write(&outByte, 1);
    if (!stream_.good()) {
        throw std::runtime_error("Failed to flush padded byte to output stream");
    }

    ++bytesWritten_;
    currentByte_ = 0;
    bitCount_ = 0;
    return paddingBits;
}

std::uint64_t BitWriter::getMeaningfulBitsWritten() const {
    return meaningfulBitsWritten_;
}

std::uint64_t BitWriter::getBytesWritten() const {
    return bytesWritten_;
}

std::uint8_t BitWriter::getPendingBitCount() const {
    return bitCount_;
}
