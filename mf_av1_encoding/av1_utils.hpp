#pragma once
#include <cstdint>
#include <span>

struct AV1Helper 
{
    // LEB128 parsing (used for OBU sizes)
    static uint64_t ReadLeb128(const std::span<const uint8_t> data, size_t& bytes_read)
    {
        uint64_t value{};
        size_t i{};
        for (; i < data.size(); ++i)
        {
            uint8_t byte = data[i];
            value |= (static_cast<uint64_t>(byte & 0x7F) << (i * 7));
            if (!(byte & 0x80))
            {
                break;
            }
        }
        bytes_read = i + 1;
        return value;
    }

    // Create the configOBUs blob from the sample (returns raw configOBUs, not the 'av1C' box)
    // This keeps responsibilities clear: helper extracts the config OBUs; container/MP4 code
    // builds the atom/box header (size + "av1C") where required.
    static std::span<const uint8_t> ExtractConfigObu(std::span<const uint8_t> samplePayload)
    {
        size_t offset{};
        while (offset < samplePayload.size())
        {
            const uint8_t obu_header = samplePayload[offset];
            const uint8_t obu_type = (obu_header >> 3) & 0xF;
            const bool obu_extension_flag = (obu_header >> 2) & 0x1;
            const bool obu_has_size_field = (obu_header >> 1) & 0x1;

            offset++;               // Move past header byte
            if (obu_extension_flag)
            {
                offset++;           // Skip extension byte
            }

            size_t obu_size{};
            size_t leb_bytes{};
            if (obu_has_size_field)
            {
                obu_size = ReadLeb128(samplePayload.subspan(offset), leb_bytes);
                offset += leb_bytes;
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
