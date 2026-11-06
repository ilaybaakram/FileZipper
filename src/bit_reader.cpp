#include "bit_reader.h"

#include <stdexcept>

BitReader::BitReader(std::istream& stream, std::uint64_t maxBitsToRead)
    : stream_(stream),
      maxBitsToRead_(maxBitsToRead),
      bitsRead_(0),
      currentByte_(0),
      bitsRemainingInByte_(0) {}

bool BitReader::readBit(int& bitOut) {
    // Respect the meaningful encoded bit count so padding bits are ignored
    if (bitsRead_ >= maxBitsToRead_) {
        return false;
    }

    // Fetch the next byte from the stream when the current byte buffer is empty
    if (bitsRemainingInByte_ == 0) {
        char rawByte = 0;
        if (!stream_.read(&rawByte, 1)) {
            if (stream_.bad()) {
                throw std::runtime_error("Stream I/O error while reading byte in BitReader");
            }
            return false; // Reached End-Of-File
        }
        currentByte_ = static_cast<std::uint8_t>(static_cast<unsigned char>(rawByte));
        bitsRemainingInByte_ = 8;
    }

    // Extract bits MSB-first (bit 7 down to bit 0), matching BitWriter::writeBit
    --bitsRemainingInByte_;
    bitOut = static_cast<int>((currentByte_ >> bitsRemainingInByte_) & 1U);
    ++bitsRead_;
    return true;
}

std::uint64_t BitReader::getBitsRead() const {
    return bitsRead_;
}

std::uint64_t BitReader::getMaxBitsToRead() const {
    return maxBitsToRead_;
}

bool BitReader::hasMoreMeaningfulBits() const {
    return bitsRead_ < maxBitsToRead_;
}

std::uint8_t BitReader::getBufferedBitCount() const {
    return bitsRemainingInByte_;
}
