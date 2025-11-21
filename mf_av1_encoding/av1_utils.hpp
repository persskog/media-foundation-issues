#pragma once
#include <vector>
#include <cstdint>
#include <span>
#include <stdexcept>

// Minimal BitReader to parse AV1 headers
class BitReader
{
    const uint8_t* m_data;
    size_t m_size;
    size_t m_bitPos = 0;

public:
    BitReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    uint32_t Read(int numBits) 
    {
        uint32_t value = 0;
        for (int i = 0; i < numBits; ++i)
        {
            size_t byteIndex = m_bitPos / 8;
            size_t bitIndex = 7 - (m_bitPos % 8);
            if (byteIndex >= m_size)
            {
                return 0; // Buffer overflow protection
            }
            uint8_t bit = (m_data[byteIndex] >> bitIndex) & 1;
            value = (value << 1) | bit;
            m_bitPos++;
        }
        return value;
    }

    void Skip(int numBits)
    {
        m_bitPos += numBits;
    }
};

// Structure to hold parsed Sequence Header info
struct AV1SeqInfo 
{
    uint8_t seq_profile;
    uint8_t seq_level_idx_0;
    uint8_t seq_tier_0;
    uint8_t high_bitdepth;
    uint8_t twelve_bit;
    uint8_t monochrome;
    uint8_t chroma_subsampling_x;
    uint8_t chroma_subsampling_y;
    uint8_t chroma_sample_position;
};

struct AV1Helper 
{
    // LEB128 parsing (used for OBU sizes)
    static uint64_t ReadLeb128(const uint8_t* data, size_t size, size_t& bytesRead)
    {
        uint64_t value = 0;
        size_t i = 0;
        for (; i < size; ++i)
        {
            uint8_t byte = data[i];
            value |= (static_cast<uint64_t>(byte & 0x7F) << (i * 7));
            if (!(byte & 0x80))
            {
                break;
            }
        }
        bytesRead = i + 1;
        return value;
    }

    // Create the configOBUs blob from the sample (returns raw configOBUs, not the 'av1C' box)
    // This keeps responsibilities clear: helper extracts the config OBUs; container/MP4 code
    // builds the atom/box header (size + "av1C") where required.
    static std::span<const uint8_t> CreateAV1C(std::span<const uint8_t> samplePayload)
    {
        size_t offset = 0;
        bool found = false;

        while (offset < samplePayload.size())
        {
            uint8_t obu_header = samplePayload[offset];
            uint8_t obu_type = (obu_header >> 3) & 0xF;
            bool obu_extension_flag = (obu_header >> 2) & 0x1;
            bool obu_has_size_field = (obu_header >> 1) & 0x1;

            offset++;               // Move past header byte
            if (obu_extension_flag)
            {
                offset++;           // Skip extension byte
            }

            size_t obu_size = 0;
            size_t lebBytes = 0;
            if (obu_has_size_field)
            {
                obu_size = ReadLeb128(samplePayload.data() + offset, samplePayload.size() - offset, lebBytes);
                offset += lebBytes;
            }
            else
            {
                obu_size = samplePayload.size() - offset;
            }

            if (obu_type == 1)
            {
                return samplePayload.subspan(offset, obu_size);
            }

            offset += obu_size;
        }

        return {};
    }
};
