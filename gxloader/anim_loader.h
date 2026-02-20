#pragma once
/// @file anim_loader.h
/// @brief GXAN animation runtime loader

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxan.h"
#include "gxmd.h"

namespace gxloader
{

struct LoadedJoint; // forward from model_loader.h

struct LoadedAnimChannelGxan
{
    std::string boneName;
    uint8_t     target = 0; // 0=T, 1=R, 2=S
    uint8_t     interpolation = 0;
    std::vector<gxfmt::VectorKey> vecKeys;
    std::vector<gxfmt::QuatKey>   quatKeys;
};

struct LoadedGxan
{
    float duration = 0.0f;
    std::vector<LoadedAnimChannelGxan> channels;
};

/// Load a GXAN animation file
std::unique_ptr<LoadedGxan> LoadGxan(const std::string& filePath);

/// Load a GXAN from memory buffer
std::unique_ptr<LoadedGxan> LoadGxanFromMemory(const uint8_t* data, size_t size);

#ifdef _WIN32
/// Load a GXAN file using wide-char path (supports non-ASCII paths on Windows)
std::unique_ptr<LoadedGxan> LoadGxanW(const std::wstring& filePath);
#endif

} // namespace gxloader
