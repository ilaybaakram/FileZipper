#include "frequency_analyzer.h"

#include <fstream>
#include <stdexcept>

FrequencyAnalyzer::FrequencyAnalyzer() : frequencies_{}, totalBytes_(0) {}

void FrequencyAnalyzer::reset() {
    frequencies_.fill(0);
    totalBytes_ = 0;
}

void FrequencyAnalyzer::analyzeFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open input file in binary mode: " + filePath);
    }
    analyzeStream(file);
    if (file.bad()) {
        throw std::runtime_error("I/O error while reading file: " + filePath);
    }
}

void FrequencyAnalyzer::analyzeStream(std::istream& stream) {
    reset();

    // Read in 4 KB chunks for efficient buffered binary I/O
    char buffer[4096];
    while (stream.read(buffer, sizeof(buffer)) || stream.gcount() > 0) {
        std::streamsize bytesRead = stream.gcount();
        for (std::streamsize i = 0; i < bytesRead; ++i) {
            unsigned char byte = static_cast<unsigned char>(buffer[i]);
            ++frequencies_[byte];
        }
        totalBytes_ += static_cast<std::uint64_t>(bytesRead);

        if (stream.eof()) {
            break;
        }
    }
}

void FrequencyAnalyzer::analyzeBuffer(const std::string& data) {
    reset();
    for (char ch : data) {
        unsigned char byte = static_cast<unsigned char>(ch);
        ++frequencies_[byte];
    }
    totalBytes_ = static_cast<std::uint64_t>(data.size());
}

const FrequencyTable& FrequencyAnalyzer::getFrequencies() const {
    return frequencies_;
}

std::uint64_t FrequencyAnalyzer::getFrequency(unsigned char byte) const {
    return frequencies_[byte];
}

std::uint64_t FrequencyAnalyzer::getTotalBytes() const {
    return totalBytes_;
}

std::size_t FrequencyAnalyzer::getUniqueByteCount() const {
    std::size_t uniqueCount = 0;
    for (std::uint64_t count : frequencies_) {
        if (count > 0) {
            ++uniqueCount;
        }
    }
    return uniqueCount;
}
