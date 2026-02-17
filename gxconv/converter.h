#pragma once
/// @file converter.h
/// @brief Conversion orchestrator

#include <string>
#include "shader_model.h"

namespace gxconv
{

struct ConvertOptions
{
    std::string inputPath;
    std::string outputPath;
    bool infoOnly        = false;
    bool useIndex16      = false;
    bool excludeAnimations = false;
    bool animOnly        = false;   // Phase 5: export animations as .gxan
    bool hasShaderModelOverride = false;
    gxfmt::ShaderModel shaderModelOverride = gxfmt::ShaderModel::Standard;
    float toonOutlineWidth = 0.0f;
};

class Converter
{
public:
    int Run(const ConvertOptions& options);

private:
    int ShowInfo(const std::string& path);
};

} // namespace gxconv
