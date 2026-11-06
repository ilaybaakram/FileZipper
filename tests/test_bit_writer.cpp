#include "bit_writer.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int main() {
    std::cout << "=== FileZipper: BitWriter Unit Tests ===" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Writing bits that form an exact full byte (8 bits)
    // -------------------------------------------------------------------------
    {
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);

        // Write 8 bits: 10101100 = 0xAC (172)
        writer.writeBits("10101100");

        assert(writer.getMeaningfulBitsWritten() == 8);
        assert(writer.getBytesWritten() == 1);
        assert(writer.getPendingBitCount() == 0);

        // Flushing when pending bit count is 0 should add 0 padding bits and write no extra byte
        std::uint8_t padding = writer.flush();
        assert(padding == 0);
        assert(writer.getBytesWritten() == 1);

        const std::string output = oss.str();
        assert(output.size() == 1);
        assert(static_cast<unsigned char>(output[0]) == 0xAC);

        std::cout << "[PASS] Test 1: Full byte (8 bits -> 0xAC) packed without extra padding." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Writing a partial byte with trailing zero-padding
    // -------------------------------------------------------------------------
    {
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);

        // Write 3 bits: "101" -> padded with 5 trailing zeros -> "10100000" = 0xA0 (160)
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(1);

        assert(writer.getMeaningfulBitsWritten() == 3);
        assert(writer.getPendingBitCount() == 3);
        assert(writer.getBytesWritten() == 0);

        std::uint8_t padding = writer.flush();
        assert(padding == 5);
        assert(writer.getPendingBitCount() == 0);
        assert(writer.getMeaningfulBitsWritten() == 3);
        assert(writer.getBytesWritten() == 1);

        const std::string output = oss.str();
        assert(output.size() == 1);
        assert(static_cast<unsigned char>(output[0]) == 0xA0);

        std::cout << "[PASS] Test 2: Partial byte ('101' -> 0xA0 with 5 padding bits) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Writing multiple full bytes followed by a partial byte
    // -------------------------------------------------------------------------
    {
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);

        // "11110000" (0xF0) + "00001111" (0x0F) + "11" (padded with 6 zeros -> "11000000" = 0xC0)
        writer.writeBits("111100000000111111");

        assert(writer.getMeaningfulBitsWritten() == 18);
        assert(writer.getBytesWritten() == 2);
        assert(writer.getPendingBitCount() == 2);

        std::uint8_t padding = writer.flush();
        assert(padding == 6);
        assert(writer.getBytesWritten() == 3);

        const std::string output = oss.str();
        assert(output.size() == 3);
        assert(static_cast<unsigned char>(output[0]) == 0xF0);
        assert(static_cast<unsigned char>(output[1]) == 0x0F);
        assert(static_cast<unsigned char>(output[2]) == 0xC0);

        std::cout << "[PASS] Test 3: Multi-byte + partial byte stream (18 bits -> 3 bytes) verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Invalid character rejection in writeBits
    // -------------------------------------------------------------------------
    {
        std::ostringstream oss(std::ios::binary);
        BitWriter writer(oss);
        bool caughtInvalidArg = false;
        try {
            writer.writeBits("01021");
        } catch (const std::invalid_argument&) {
            caughtInvalidArg = true;
        }
        assert(caughtInvalidArg == true);
        std::cout << "[PASS] Test 4: Invalid bit character rejection verified." << std::endl;
    }

    std::cout << "All BitWriter tests passed successfully!" << std::endl;
    return 0;
}
