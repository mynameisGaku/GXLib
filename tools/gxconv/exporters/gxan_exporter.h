#pragma once
/// @file gxan_exporter.h
/// @brief GXAN standalone animation exporter

#include <string>

namespace gxconv
{

struct Scene;

class GxanExporter
{
public:
    bool Export(const Scene& scene, const std::string& outputPath);
};

} // namespace gxconv
