#pragma once
/// @file gxmd_exporter.h
/// @brief GXMD binary model exporter

#include <string>

namespace gxconv
{

struct Scene;

struct ExportOptions
{
    bool useIndex16 = false;     // force 16-bit indices
    bool excludeAnimations = false;
};

class GxmdExporter
{
public:
    bool Export(const Scene& scene, const std::string& outputPath,
               const ExportOptions& options = {});
};

} // namespace gxconv
