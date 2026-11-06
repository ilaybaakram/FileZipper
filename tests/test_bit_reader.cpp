#include "bit_reader.h"
#include "bit_writer.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::cout << "=== FileZipper: BitReader Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Reading a complete byte (8 bits) MSB-first
    // -------------------------------------------------------------------------
    {
        // 0xAC = 10101100 in binary
        const std::string rawData(1, static_cast<char>(0xAC));
        std::istringstream iss(rawData, std::ios::binary);
        BitReader reader(iss, 8);

        std::string bits;
        int bit = 0;
        while (reader.readBit(bit)) {
            bits.push_back(bit ? '1' : '0');
        }

        assert(bits == "10101100");
        assert(reader.getBitsRead() == 8);
        assert(!reader.hasMoreMeaningfulBits());
        assert(reader.readBit(bit) == false);

        std::cout << "[PASS] Test 1: Reading a complete byte (0xAC -> '10101100') verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Reading partial bit sequences & respecting meaningful bit count
    //         (ignoring trailing padding bits)
    // -------------------------------------------------------------------------
    {
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);
        writer.writeBits("10110"); // 5 meaningful bits + 3 zero-padding bits
        std::uint8_t padding = writer.flush();
        assert(padding == 3);

        std::istringstream iss(oss.str(), std::ios::binary);
        BitReader reader(iss, writer.getMeaningfulBitsWritten());

        std::string readBack;
        int bit = 0;
        while (reader.readBit(bit)) {
            readBack.push_back(bit ? '1' : '0');
        }

        assert(readBack == "10110");
        assert(reader.getBitsRead() == 5);
        assert(reader.getBufferedBitCount() == 3); // 3 padding bits left unread
        assert(reader.readBit(bit) == false);      // Refuses to read padding bits

        std::cout << "[PASS] Test 2: Partial bit sequence ('10110') read without consuming 3 padding bits." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Reading multiple bytes + partial byte round-trip with BitWriter
    // -------------------------------------------------------------------------
    {
        const std::string expectedBits = "1100101011110000010101"; // 22 bits (2 full bytes + 6 bits)
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);
        writer.writeBits(expectedBits);
        writer.flush();

        std::istringstream iss(oss.str(), std::ios::binary);
        BitReader reader(iss, expectedBits.size());

        std::string actualBits;
        int bit = 0;
        while (reader.readBit(bit)) {
            actualBits.push_back(bit ? '1' : '0');
        }

        assert(actualBits == expectedBits);
        assert(reader.getBitsRead() == 22);

        std::cout << "[PASS] Test 3: Multi-byte bitstream (22 bits across 3 bytes) round-trip verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Safe End-Of-Stream (EOF) handling when stream ends early
    // -------------------------------------------------------------------------
    {
        const std::string singleByte(1, static_cast<char>(0xFF));
        std::istringstream iss(singleByte, std::ios::binary);
        // Request 16 bits from a 1-byte (8-bit) stream
        BitReader reader(iss, 16);

        int count = 0;
        int bit = 0;
        while (reader.readBit(bit)) {
            assert(bit == 1);
            ++count;
        }
        // Should safely stop at 8 bits when EOF is reached
        assert(count == 8);
        assert(reader.getBitsRead() == 8);

        std::cout << "[PASS] Test 4: Safe End-Of-Stream (EOF) handling verified." << std::endl;
    }

    std::cout << "All BitReader unit tests passed successfully!" << std::endl;
    return 0;
}
