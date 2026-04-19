/// @file WeatherMap.cpp
/// @brief WeatherMap の実装 — 2D RGB weather texture + JSON IO + GPU upload (ADR-0021).

#include "pch_common.h"
#include "Graphics/3D/WeatherMap.h"
#include "Graphics/Resource/TextureManager.h"
#include "Core/Logger.h"
#include "ThirdParty/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace gx
{

using json = nlohmann::json;

// ============================================================================
// Lifecycle
// ============================================================================

bool WeatherMap::Initialize(uint32_t width, uint32_t height, TextureManager* textureManager)
{
    if (width == 0 || height == 0)
    {
        GX_LOG_ERROR("WeatherMap::Initialize - invalid size %ux%u", width, height);
        return false;
    }

    m_width = width;
    m_height = height;
    m_pixels.assign(static_cast<size_t>(width) * height * 4, 0);
    // Alpha channel filled to 255 (reserved)
    for (size_t i = 3; i < m_pixels.size(); i += 4)
        m_pixels[i] = 255;

    m_textureManager = textureManager;
    m_textureHandle = -1;
    m_initialized = true;
    m_dirty = true;

    // Eagerly upload if texture manager provided
    if (m_textureManager)
        UploadToGPU();

    return true;
}

void WeatherMap::BindTextureManager(TextureManager* textureManager)
{
    m_textureManager = textureManager;
    // Re-upload on next UploadToGPU call
    m_dirty = true;
    // If we already had a handle from a prior TextureManager, it's orphaned —
    // caller is responsible for lifecycle on change-over. Null it out here.
    m_textureHandle = -1;
}

void WeatherMap::Shutdown()
{
    if (m_textureManager && m_textureHandle >= 0)
    {
        m_textureManager->ReleaseTexture(m_textureHandle);
    }
    m_textureHandle = -1;
    m_textureManager = nullptr;
    m_pixels.clear();
    m_width = 0;
    m_height = 0;
    m_initialized = false;
    m_dirty = true;
}

// ============================================================================
// Cell access
// ============================================================================

WeatherCell WeatherMap::Sample(uint32_t x, uint32_t y) const
{
    if (!m_initialized || x >= m_width || y >= m_height)
        return WeatherCell{};  // clear

    size_t off = PixelOffset(x, y);
    return WeatherCell{
        m_pixels[off + 0],
        m_pixels[off + 1],
        m_pixels[off + 2],
        m_pixels[off + 3]
    };
}

void WeatherMap::SetCell(uint32_t x, uint32_t y, const WeatherCell& cell)
{
    if (!m_initialized || x >= m_width || y >= m_height) return;

    size_t off = PixelOffset(x, y);
    m_pixels[off + 0] = cell.coverage;
    m_pixels[off + 1] = cell.precipitation;
    m_pixels[off + 2] = cell.cloudType;
    m_pixels[off + 3] = cell.reserved;
    m_dirty = true;
}

void WeatherMap::SetRegion(const WeatherRegion& region, const WeatherCell& cell)
{
    if (!m_initialized) return;

    uint32_t x0 = (std::min)(region.x, m_width);
    uint32_t y0 = (std::min)(region.y, m_height);
    uint32_t x1 = (std::min)(region.x + region.width, m_width);
    uint32_t y1 = (std::min)(region.y + region.height, m_height);

    for (uint32_t y = y0; y < y1; ++y)
    {
        for (uint32_t x = x0; x < x1; ++x)
        {
            size_t off = PixelOffset(x, y);
            m_pixels[off + 0] = cell.coverage;
            m_pixels[off + 1] = cell.precipitation;
            m_pixels[off + 2] = cell.cloudType;
            m_pixels[off + 3] = cell.reserved;
        }
    }
    m_dirty = true;
}

void WeatherMap::Fill(const WeatherCell& cell)
{
    if (!m_initialized) return;

    for (size_t i = 0; i < m_pixels.size(); i += 4)
    {
        m_pixels[i + 0] = cell.coverage;
        m_pixels[i + 1] = cell.precipitation;
        m_pixels[i + 2] = cell.cloudType;
        m_pixels[i + 3] = cell.reserved;
    }
    m_dirty = true;
}

WeatherCell WeatherMap::SampleWorld(float worldX, float worldZ) const
{
    if (!m_initialized || m_worldExtent <= 0.0f) return WeatherCell{};

    // Map world(-extent/2 .. +extent/2) to cell(0 .. width)
    float half = m_worldExtent * 0.5f;
    float u = (worldX + half) / m_worldExtent;  // 0..1
    float v = (worldZ + half) / m_worldExtent;

    // Wrap (not clamp) so flight far from origin still gets varied clouds
    auto wrap01 = [](float t) {
        t = std::fmod(t, 1.0f);
        if (t < 0.0f) t += 1.0f;
        return t;
    };
    u = wrap01(u);
    v = wrap01(v);

    uint32_t x = static_cast<uint32_t>(u * m_width);
    uint32_t y = static_cast<uint32_t>(v * m_height);
    if (x >= m_width)  x = m_width - 1;
    if (y >= m_height) y = m_height - 1;
    return Sample(x, y);
}

// ============================================================================
// GPU upload
// ============================================================================

bool WeatherMap::UploadToGPU()
{
    if (!m_initialized || !m_textureManager)
        return false;

    if (m_textureHandle < 0)
    {
        // First upload — create new texture
        m_textureHandle = m_textureManager->CreateTextureFromMemory(
            m_pixels.data(), m_width, m_height);
        if (m_textureHandle < 0)
        {
            GX_LOG_ERROR("WeatherMap::UploadToGPU - CreateTextureFromMemory failed");
            return false;
        }
    }
    else
    {
        // Re-upload to existing texture (hot reload / SetCell 後)
        if (!m_textureManager->UpdateTextureFromMemory(
                m_textureHandle, m_pixels.data(), m_width, m_height))
        {
            GX_LOG_ERROR("WeatherMap::UploadToGPU - UpdateTextureFromMemory failed (handle=%d)",
                         m_textureHandle);
            return false;
        }
    }

    m_dirty = false;
    return true;
}

// ============================================================================
// JSON IO (fill + regions encoding、artist-authorable)
// ============================================================================

bool WeatherMap::LoadFromJSON(const gx::String& path)
{
    std::ifstream f(path.c_str());
    if (!f.is_open())
    {
        GX_LOG_ERROR("WeatherMap::LoadFromJSON - open failed: %s", path.c_str());
        return false;
    }

    json j;
    try { f >> j; }
    catch (const std::exception& ex) {
        GX_LOG_ERROR("WeatherMap::LoadFromJSON - parse error: %s (%s)", path.c_str(), ex.what());
        return false;
    }

    // Schema check
    std::string schema = j.value("schema", "");
    if (schema != "gx-weathermap-1") {
        GX_LOG_ERROR("WeatherMap::LoadFromJSON - unknown schema '%s' (expected 'gx-weathermap-1')",
                     schema.c_str());
        return false;
    }

    uint32_t w = j.value("width", k_DefaultSize);
    uint32_t h = j.value("height", k_DefaultSize);
    float extent = j.value("worldExtent", k_DefaultWorldExtent);

    // Init with same TextureManager binding (may be nullptr, caller re-uploads)
    TextureManager* tm = m_textureManager;
    Initialize(w, h, tm);
    m_worldExtent = extent;

    // Apply fill
    if (j.contains("fill"))
    {
        const auto& fillJ = j["fill"];
        WeatherCell fillCell{
            fillJ.value("r", 0),
            fillJ.value("g", 0),
            fillJ.value("b", 0),
            fillJ.value("a", 255)
        };
        Fill(fillCell);
    }

    // Apply regions
    if (j.contains("regions"))
    {
        for (const auto& r : j["regions"])
        {
            WeatherRegion region{
                r.value("x", 0u),
                r.value("y", 0u),
                r.value("w", 1u),
                r.value("h", 1u)
            };
            WeatherCell cell{
                r.value("r", 0),
                r.value("g", 0),
                r.value("b", 0),
                r.value("a", 255)
            };
            SetRegion(region, cell);
        }
    }

    m_dirty = true;
    if (m_textureManager) UploadToGPU();
    GX_LOG_INFO("WeatherMap loaded: %s (%ux%u, extent=%.1fm)",
                path.c_str(), m_width, m_height, m_worldExtent);
    return true;
}

bool WeatherMap::SaveToJSON(const gx::String& path) const
{
    if (!m_initialized)
    {
        GX_LOG_ERROR("WeatherMap::SaveToJSON - not initialized");
        return false;
    }

    // ADR-0019 §5 atomicity invariant: write to tmp, rename atomic.
    const std::string finalPath(path.c_str());
    const std::string tmpPath = finalPath + ".tmp";

    // Encode: simplest dense form — per-cell list. (Could auto-detect fill + regions
    // for a more compact format, but Phase B keeps it straightforward; presets can
    // be hand-authored in the fill+regions style and loaded, while runtime-saved
    // maps use dense to preserve per-cell edits.)
    //
    // For 512² at ~40 bytes per JSON line, that is ~10 MB per save — acceptable for
    // hobby authoring. Compact binary format can come in Phase F.
    json j;
    j["schema"] = "gx-weathermap-1";
    j["width"] = m_width;
    j["height"] = m_height;
    j["worldExtent"] = m_worldExtent;

    // Sample first cell as default "fill" hint (optional, readers ignore)
    if (!m_pixels.empty())
    {
        j["fill"] = {
            { "r", m_pixels[0] },
            { "g", m_pixels[1] },
            { "b", m_pixels[2] },
            { "a", m_pixels[3] }
        };
    }

    // Only emit regions for cells differing from fill
    json regions = json::array();
    if (!m_pixels.empty())
    {
        uint8_t fr = m_pixels[0], fg = m_pixels[1], fb = m_pixels[2], fa = m_pixels[3];
        for (uint32_t y = 0; y < m_height; ++y)
        {
            for (uint32_t x = 0; x < m_width; ++x)
            {
                size_t off = PixelOffset(x, y);
                if (m_pixels[off + 0] != fr || m_pixels[off + 1] != fg ||
                    m_pixels[off + 2] != fb || m_pixels[off + 3] != fa)
                {
                    regions.push_back({
                        { "x", x }, { "y", y }, { "w", 1u }, { "h", 1u },
                        { "r", m_pixels[off + 0] },
                        { "g", m_pixels[off + 1] },
                        { "b", m_pixels[off + 2] },
                        { "a", m_pixels[off + 3] }
                    });
                }
            }
        }
    }
    j["regions"] = regions;

    auto cleanupTmp = [&tmpPath]() {
        std::error_code ec;
        std::filesystem::remove(tmpPath, ec);
    };

    {
        std::ofstream f(tmpPath);
        if (!f.is_open())
        {
            GX_LOG_ERROR("WeatherMap::SaveToJSON - tmp open failed: %s", tmpPath.c_str());
            return false;
        }
        f << j.dump(2);
        f.flush();
        if (!f.good())
        {
            f.close();
            cleanupTmp();
            return false;
        }
    } // RAII close

    std::error_code ec;
    std::filesystem::rename(tmpPath, finalPath, ec);
    if (ec)
    {
        cleanupTmp();
        GX_LOG_ERROR("WeatherMap::SaveToJSON - rename failed: %s", ec.message().c_str());
        return false;
    }
    return true;
}

} // namespace gx
