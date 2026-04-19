/// @file main.cpp
/// @brief 17-cloud-weather-preset — WeatherMap JSON load + hot-reload demo (ADR-0021 Phase B).
///
/// Demonstrates Phase B data model authoring loop:
///   - 5 weather presets authored as JSON in Assets/weather/
///   - Number keys 1-5 switch presets at runtime
///   - FileWatcher on Assets/weather/ reloads current preset on disk change
///     (edit the JSON in a text editor while this example is running → clouds update)
///
/// BRIDGE NOTE: This example uses the existing procedural VolumetricClouds
/// as the rendering backend, driving it via aggregate stats (avg coverage,
/// avg cloud type) computed from the WeatherMap. Phase B remaining task is
/// refactoring the shader to sample WeatherMap directly — after that lands,
/// this same example will show per-region cloud variation instead of a
/// global coverage slider. The bridge lets you verify authoring + hot-reload
/// today without waiting for the shader work.

#include "GXLib.h"
#include "Graphics/3D/WeatherMap.h"
#include "Graphics/3D/CloudField.h"
#include "Graphics/3D/CloudLayer.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/3D/VolumetricClouds.h"
#include "IO/FileWatcher.h"
#include <cstring>

static const char* const k_Presets[] = {
    "Assets/weather/clear.json",
    "Assets/weather/overcast.json",
    "Assets/weather/scattered_cumulus.json",
    "Assets/weather/storm_front.json",
    "Assets/weather/heavy_storm.json"
};
static const char* const k_PresetNames[] = {
    "clear", "overcast", "scattered_cumulus", "storm_front", "heavy_storm"
};
static constexpr int k_PresetCount = sizeof(k_Presets) / sizeof(k_Presets[0]);

// Bridge: compute aggregate stats from a WeatherMap and apply to legacy VolumetricClouds API.
// Phase B shader refactor will replace this with direct WeatherMap sampling per fragment.
struct WeatherStats {
    float avgCoverage    = 0.0f;    // 0..1
    float avgCloudType   = 0.0f;    // 0..1
    float avgPrecipitation = 0.0f;  // 0..1
    uint32_t cellCount   = 0;
    float coverageVariance = 0.0f;  // used as CoverageVariation
};

static WeatherStats ComputeStats(const gx::WeatherMap& map)
{
    WeatherStats s;
    const uint32_t w = map.GetWidth();
    const uint32_t h = map.GetHeight();
    if (w == 0 || h == 0) return s;

    s.cellCount = w * h;
    float sumCov = 0, sumType = 0, sumPrec = 0;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            auto c = map.Sample(x, y);
            sumCov  += c.coverage      / 255.0f;
            sumType += c.cloudType     / 255.0f;
            sumPrec += c.precipitation / 255.0f;
        }
    }
    const float n = static_cast<float>(s.cellCount);
    s.avgCoverage      = sumCov / n;
    s.avgCloudType     = sumType / n;
    s.avgPrecipitation = sumPrec / n;

    // Variance of coverage (indicates scattered vs uniform)
    float sumVar = 0;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            float d = (map.Sample(x, y).coverage / 255.0f) - s.avgCoverage;
            sumVar += d * d;
        }
    }
    s.coverageVariance = std::sqrt(sumVar / n);
    return s;
}

static bool LoadPreset(int index, gx::WeatherMap& map, WeatherStats& outStats)
{
    if (index < 0 || index >= k_PresetCount) return false;
    if (!map.LoadFromJSON(gx::String(k_Presets[index]))) return false;
    outStats = ComputeStats(map);
    return true;
}

static void ApplyStatsToClouds(const WeatherStats& stats)
{
    auto& pipe = gx::GetPostEffects();
    auto& clouds = pipe.GetVolumetricClouds();
    // Bridge: aggregate WeatherMap stats → legacy VolumetricClouds params
    clouds.SetCoverage(stats.avgCoverage);
    clouds.SetCoverageVariation(std::clamp(stats.coverageVariance * 4.0f, 0.0f, 1.0f));
    clouds.SetCloudType(stats.avgCloudType);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("17 - Cloud Weather Preset (ADR-0021 Phase B demo)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // WeatherMap data model (ADR-0021 Phase B)
    // =========================================================================
    gx::WeatherMap weatherMap;
    weatherMap.Initialize(128, 128, nullptr);  // CPU-only for now; bind TextureManager when shader uses it

    WeatherStats stats;
    int currentPreset = 0;
    if (!LoadPreset(currentPreset, weatherMap, stats)) {
        // Fall back to empty init
        currentPreset = -1;
    } else {
        ApplyStatsToClouds(stats);
    }

    // =========================================================================
    // FileWatcher hot-reload on Assets/weather/
    // =========================================================================
    gx::FileWatcher watcher;
    watcher.Watch(
        gx::String("Assets/weather"),
        [&weatherMap, &stats, &currentPreset](const gx::String& path) {
            // Check whether the changed file is the current preset (any of the 5 JSONs).
            // FileWatcher reports filename only (not full path); match by suffix.
            const char* changed = path.c_str();
            for (int i = 0; i < k_PresetCount; ++i) {
                const char* presetFile = std::strrchr(k_Presets[i], '/');
                presetFile = presetFile ? presetFile + 1 : k_Presets[i];
                if (std::strstr(changed, presetFile) != nullptr) {
                    if (i == currentPreset) {
                        // Reload current
                        if (LoadPreset(i, weatherMap, stats)) {
                            ApplyStatsToClouds(stats);
                        }
                    }
                    break;
                }
            }
        });

    // =========================================================================
    // Colors
    // =========================================================================
    unsigned int white = GetColor(255, 255, 255);
    unsigned int green = GetColor(80, 220, 80);
    unsigned int red   = GetColor(220, 80, 80);
    unsigned int cyan  = GetColor(100, 220, 255);
    unsigned int gray  = GetColor(160, 160, 160);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        // Pump watcher (delivers change callbacks on main thread)
        watcher.Update();

        // Preset switch keys
        int requested = -1;
        if (CheckHitKey(KEY_INPUT_1)) requested = 0;
        else if (CheckHitKey(KEY_INPUT_2)) requested = 1;
        else if (CheckHitKey(KEY_INPUT_3)) requested = 2;
        else if (CheckHitKey(KEY_INPUT_4)) requested = 3;
        else if (CheckHitKey(KEY_INPUT_5)) requested = 4;
        if (requested >= 0 && requested != currentPreset) {
            if (LoadPreset(requested, weatherMap, stats)) {
                currentPreset = requested;
                ApplyStatsToClouds(stats);
            }
        }

        ClearDrawScreen();

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawString(10, 40, "--- WeatherMap preset ---", cyan);
        if (currentPreset >= 0) {
            DrawFormatString(10, 60, green, "Current: %s (press 1-5 to switch)",
                             k_PresetNames[currentPreset]);
        } else {
            DrawString(10, 60, "No preset loaded (JSON load failed, check CWD)", red);
        }

        // Preset menu
        for (int i = 0; i < k_PresetCount; ++i) {
            unsigned int col = (i == currentPreset) ? green : gray;
            DrawFormatString(10, 90 + i * 18, col, "  [%d] %s", i + 1, k_PresetNames[i]);
        }

        // Stats panel
        DrawString(10, 200, "--- Aggregate stats (applied to VolumetricClouds) ---", cyan);
        DrawFormatString(10, 220, white, "Coverage avg      : %.3f  (%ux%u cells)",
                         stats.avgCoverage, weatherMap.GetWidth(), weatherMap.GetHeight());
        DrawFormatString(10, 240, white, "CoverageVariation : %.3f",
                         std::min(stats.coverageVariance * 4.0f, 1.0f));
        DrawFormatString(10, 260, white, "CloudType avg     : %.3f  (0=stratus, 1=cumulonimbus)",
                         stats.avgCloudType);
        DrawFormatString(10, 280, white, "Precipitation avg : %.3f",
                         stats.avgPrecipitation);

        // Hot-reload hint
        DrawString(10, 320, "--- Hot-reload ---", cyan);
        DrawString(10, 340, "Edit any Assets/weather/*.json while the app is running.", white);
        DrawString(10, 360, "Current preset will reload + clouds update on save.", white);
        DrawString(10, 380, "(FileWatcher uses ReadDirectoryChangesW; save triggers callback)", gray);

        // Bridge note
        DrawString(10, 430, "--- Phase B bridge note ---", cyan);
        DrawString(10, 450, "Rendering uses legacy VolumetricClouds driven by aggregate stats.", white);
        DrawString(10, 470, "Per-region WeatherMap sampling in the shader is the next", white);
        DrawString(10, 490, "Phase B task — after that, clouds will vary spatially (scattered", white);
        DrawString(10, 510, "cumulus will show gaps), not just get thicker/thinner uniformly.", white);

        DrawString(10, 560, "ESC: quit", white);

        ScreenFlip();
    }

    watcher.Stop();
    weatherMap.Shutdown();
    GX_End();
    return 0;
}
