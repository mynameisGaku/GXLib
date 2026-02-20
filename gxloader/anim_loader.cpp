/// @file anim_loader.cpp
/// @brief GXAN animation runtime loader implementation

#include "anim_loader.h"
#include <cstdio>
#include <cstring>

namespace gxloader
{

static std::string ReadString(const uint8_t* stringData, uint32_t stringSize, uint32_t offset)
{
    if (offset == gxfmt::k_InvalidStringIndex || offset >= stringSize)
        return "";
    return std::string(reinterpret_cast<const char*>(stringData + offset));
}

std::unique_ptr<LoadedGxan> LoadGxanFromMemory(const uint8_t* data, size_t size)
{
    if (size < sizeof(gxfmt::GxanHeader))
        return nullptr;

    const auto* header = reinterpret_cast<const gxfmt::GxanHeader*>(data);
    if (header->magic != gxfmt::k_GxanMagic)
        return nullptr;

    auto anim = std::make_unique<LoadedGxan>();
    anim->duration = header->duration;

    // Read string table
    const uint8_t* stBase = data + header->stringTableOffset;
    uint32_t stSize = 0;
    std::memcpy(&stSize, stBase, 4);
    const uint8_t* stringData = stBase + 4;

    // Read channel descriptors
    const auto* descs = reinterpret_cast<const gxfmt::GxanChannelDesc*>(data + header->channelDescOffset);
    const uint8_t* keyBase = data + header->keyDataOffset;

    for (uint32_t ci = 0; ci < header->channelCount; ++ci)
    {
        const auto& desc = descs[ci];
        LoadedAnimChannelGxan ch;
        ch.boneName = ReadString(stringData, stSize, desc.boneNameIndex);
        ch.target = desc.target;
        ch.interpolation = desc.interpolation;

        const uint8_t* keyPtr = keyBase + desc.dataOffset;

        if (desc.target == 1) // rotation
        {
            ch.quatKeys.resize(desc.keyCount);
            std::memcpy(ch.quatKeys.data(), keyPtr, desc.keyCount * sizeof(gxfmt::QuatKey));
        }
        else
        {
            ch.vecKeys.resize(desc.keyCount);
            std::memcpy(ch.vecKeys.data(), keyPtr, desc.keyCount * sizeof(gxfmt::VectorKey));
        }

        anim->channels.push_back(std::move(ch));
    }

    return anim;
}

std::unique_ptr<LoadedGxan> LoadGxan(const std::string& filePath)
{
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) { fclose(f); return nullptr; }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    fread(buffer.data(), 1, buffer.size(), f);
    fclose(f);

    return LoadGxanFromMemory(buffer.data(), buffer.size());
}

#ifdef _WIN32
std::unique_ptr<LoadedGxan> LoadGxanW(const std::wstring& filePath)
{
    FILE* f = _wfopen(filePath.c_str(), L"rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) { fclose(f); return nullptr; }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    fread(buffer.data(), 1, buffer.size(), f);
    fclose(f);

    return LoadGxanFromMemory(buffer.data(), buffer.size());
}
#endif

} // namespace gxloader
