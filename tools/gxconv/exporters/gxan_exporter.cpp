/// @file gxan_exporter.cpp
/// @brief GXAN standalone animation exporter implementation

#include "gxan_exporter.h"
#include "intermediate/scene.h"
#include "gxan.h"
#include "gxmd.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <map>

namespace gxconv
{

class StringTableBuilder
{
public:
    uint32_t Add(const std::string& str)
    {
        if (str.empty()) return gxfmt::k_InvalidStringIndex;
        auto it = m_map.find(str);
        if (it != m_map.end()) return it->second;
        uint32_t offset = static_cast<uint32_t>(m_data.size());
        m_map[str] = offset;
        m_data.insert(m_data.end(), str.begin(), str.end());
        m_data.push_back('\0');
        return offset;
    }
    const std::vector<uint8_t>& Data() const { return m_data; }
    uint32_t Size() const { return static_cast<uint32_t>(m_data.size()); }
private:
    std::vector<uint8_t> m_data;
    std::map<std::string, uint32_t> m_map;
};

bool GxanExporter::Export(const Scene& scene, const std::string& outputPath)
{
    if (scene.animations.empty())
    {
        fprintf(stderr, "Error: No animations to export\n");
        return false;
    }

    // Use first animation
    const auto& anim = scene.animations[0];

    StringTableBuilder strings;

    // Flatten intermediate channels into GXAN channels (bone name based)
    struct GxanChannel
    {
        uint32_t boneNameIndex;
        uint8_t target;
        uint8_t interpolation;
        uint32_t dataOffset; // computed later
        std::vector<gxfmt::VectorKey> vecKeys;
        std::vector<gxfmt::QuatKey> quatKeys;
    };

    std::vector<GxanChannel> channels;

    for (auto& srcCh : anim.channels)
    {
        GxanChannel ch;

        // Use bone name. If empty, try to find from skeleton
        std::string boneName = srcCh.boneName;
        if (boneName.empty() && srcCh.jointIndex < scene.skeleton.size())
            boneName = scene.skeleton[srcCh.jointIndex].name;

        ch.boneNameIndex = strings.Add(boneName);
        ch.target = srcCh.target;
        ch.interpolation = srcCh.interpolation;
        ch.dataOffset = 0;

        if (srcCh.target == 1) // rotation
        {
            for (auto& k : srcCh.quatKeys)
            {
                gxfmt::QuatKey qk;
                qk.time = k.time;
                std::memcpy(qk.value, k.value, 16);
                ch.quatKeys.push_back(qk);
            }
        }
        else
        {
            for (auto& k : srcCh.vecKeys)
            {
                gxfmt::VectorKey vk;
                vk.time = k.time;
                std::memcpy(vk.value, k.value, 12);
                ch.vecKeys.push_back(vk);
            }
        }

        channels.push_back(std::move(ch));
    }

    // Compute layout
    uint64_t cursor = sizeof(gxfmt::GxanHeader);

    uint64_t stringTableOffset = cursor;
    uint32_t stringTableSize = strings.Size();
    cursor += 4 + stringTableSize;

    uint64_t channelDescOffset = cursor;
    cursor += channels.size() * sizeof(gxfmt::GxanChannelDesc);

    uint64_t keyDataOffset = cursor;

    // Compute key data offsets
    uint32_t keyOffset = 0;
    for (auto& ch : channels)
    {
        ch.dataOffset = keyOffset;
        if (ch.target == 1)
            keyOffset += static_cast<uint32_t>(ch.quatKeys.size() * sizeof(gxfmt::QuatKey));
        else
            keyOffset += static_cast<uint32_t>(ch.vecKeys.size() * sizeof(gxfmt::VectorKey));
    }
    uint32_t keyDataSize = keyOffset;

    // Write
    FILE* f = fopen(outputPath.c_str(), "wb");
    if (!f) { fprintf(stderr, "Error: Cannot open %s\n", outputPath.c_str()); return false; }

    gxfmt::GxanHeader header{};
    header.magic = gxfmt::k_GxanMagic;
    header.version = gxfmt::k_GxanVersion;
    header.channelCount = static_cast<uint32_t>(channels.size());
    header.duration = anim.duration;
    header.stringTableOffset = stringTableOffset;
    header.stringTableSize = stringTableSize;
    header.channelDescOffset = channelDescOffset;
    header.keyDataOffset = keyDataOffset;
    header.keyDataSize = keyDataSize;
    fwrite(&header, sizeof(header), 1, f);

    // String table
    fwrite(&stringTableSize, 4, 1, f);
    fwrite(strings.Data().data(), 1, strings.Data().size(), f);

    // Channel descriptors
    for (auto& ch : channels)
    {
        gxfmt::GxanChannelDesc desc{};
        desc.boneNameIndex = ch.boneNameIndex;
        desc.target = ch.target;
        desc.interpolation = ch.interpolation;
        desc.dataOffset = ch.dataOffset;
        desc.keyCount = (ch.target == 1)
            ? static_cast<uint32_t>(ch.quatKeys.size())
            : static_cast<uint32_t>(ch.vecKeys.size());
        fwrite(&desc, sizeof(desc), 1, f);
    }

    // Key data
    for (auto& ch : channels)
    {
        if (ch.target == 1)
            fwrite(ch.quatKeys.data(), sizeof(gxfmt::QuatKey), ch.quatKeys.size(), f);
        else
            fwrite(ch.vecKeys.data(), sizeof(gxfmt::VectorKey), ch.vecKeys.size(), f);
    }

    fclose(f);

    printf("Exported GXAN: %s\n", outputPath.c_str());
    printf("  Duration: %.3f sec, Channels: %u\n", anim.duration, header.channelCount);

    return true;
}

} // namespace gxconv
