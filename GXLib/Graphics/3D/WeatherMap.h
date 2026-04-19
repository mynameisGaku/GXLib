#pragma once
/// @file WeatherMap.h
/// @brief Weather map — 2D RGB texture driving regional cloud coverage/precipitation/type (ADR-0021).
///
/// ADR-0021 §Decision 1 foundation. R=coverage, G=precipitation, B=cloud-type.
/// World-space 2D tiled grid (default 512×512 covering 20 km²).
/// JSON load/save with fill+regions encoding (compact、artist-authorable).
/// GPU upload via TextureManager (`int` handle、ADR-0002 準拠)。
///
/// @addtogroup grp_gfx_3d
/// @{

#include "pch_common.h"

namespace gx
{

class TextureManager;

/// @brief Per-cell weather state (RGBA8 packed as uint8_t)
struct WeatherCell
{
    uint8_t coverage      = 0;    ///< R: 雲量 (0=晴、255=overcast)
    uint8_t precipitation = 0;    ///< G: 降水量 (0=無、255=豪雨/豪雪)
    uint8_t cloudType     = 0;    ///< B: 雲種 (0=Stratus gradient、255=Cumulonimbus gradient)
    uint8_t reserved      = 255;  ///< A: reserved (常に 255 に固定、将来の拡張 e.g. temperature)

    /// @brief 便利コンストラクタ
    static WeatherCell Make(float coverage01, float precipitation01, float cloudType01)
    {
        return WeatherCell{
            static_cast<uint8_t>(std::clamp(coverage01, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(precipitation01, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(cloudType01, 0.0f, 1.0f) * 255.0f),
            255
        };
    }
};

/// @brief Weather map region (for batch SetRegion)
struct WeatherRegion
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 1;
    uint32_t height = 1;
};

/// @brief 2D weather map (ADR-0021 first-class data structure)
class WeatherMap
{
public:
    static constexpr uint32_t k_DefaultSize = 512;
    static constexpr float    k_DefaultWorldExtent = 20000.0f;  ///< 20 km²

    WeatherMap() = default;
    ~WeatherMap() = default;
    WeatherMap(const WeatherMap&) = delete;
    WeatherMap& operator=(const WeatherMap&) = delete;
    WeatherMap(WeatherMap&&) = default;
    WeatherMap& operator=(WeatherMap&&) = default;

    /// @brief 初期化 (全 cell を clear 状態 coverage=0 にする)
    /// @param width 解像度 (default 512)
    /// @param height 解像度 (default 512)
    /// @param textureManager GPU upload 用 (nullptr なら CPU-only、後から Bind できる)
    /// @return 成功で true
    bool Initialize(uint32_t width = k_DefaultSize,
                    uint32_t height = k_DefaultSize,
                    TextureManager* textureManager = nullptr);

    /// @brief TextureManager を後から紐付け (Initialize で nullptr 指定した場合)
    void BindTextureManager(TextureManager* textureManager);

    /// @brief Cleanup (Initialize 前状態に戻す、テクスチャも release)
    void Shutdown();

    /// @brief 初期化済みか
    bool IsInitialized() const { return m_initialized; }

    /// @brief 解像度 (cells)
    uint32_t GetWidth() const  { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    /// @brief 世界座標スケール (m) — SampleWorld が world(x,z)→cell 変換に使う
    void SetWorldExtent(float meters) { m_worldExtent = meters; }
    float GetWorldExtent() const       { return m_worldExtent; }

    /// @brief Cell read (bounds out なら clear cell を返す)
    WeatherCell Sample(uint32_t x, uint32_t y) const;

    /// @brief Cell write (bounds out は ignore、dirty フラグ立て)
    void SetCell(uint32_t x, uint32_t y, const WeatherCell& cell);

    /// @brief Region 一括 write (bounds は自動 clamp)
    void SetRegion(const WeatherRegion& region, const WeatherCell& cell);

    /// @brief 全 cell を同一値で埋める (clear preset 等)
    void Fill(const WeatherCell& cell);

    /// @brief 世界座標 (XZ 平面) 相対で sample (wrap)
    /// @param worldX 世界座標 X (m、中心 0 基準)
    /// @param worldZ 世界座標 Z (m、中心 0 基準)
    WeatherCell SampleWorld(float worldX, float worldZ) const;

    /// @brief JSON load (ADR-0007 asset flow、fill+regions 形式)
    /// @param path Absolute or VFS path
    bool LoadFromJSON(const gx::String& path);

    /// @brief JSON save (ADR-0019 §5 atomic tmp+rename pattern)
    /// @param path Destination
    /// @return 成功で true、destination untouched on failure
    bool SaveToJSON(const gx::String& path) const;

    /// @brief GPU texture handle (TextureManager-issued int、shader SRV sampling 用)
    /// @return -1 if not uploaded or no TextureManager bound
    int GetGPUTextureHandle() const { return m_textureHandle; }

    /// @brief CPU-side 変更を GPU texture へ upload
    /// @return 成功で true (TextureManager 未 bind or init 失敗で false)
    bool UploadToGPU();

    /// @brief CPU-side 変更未反映か (SetCell/SetRegion/Fill/LoadFromJSON 後に true)
    bool IsDirty() const { return m_dirty; }

    /// @brief 生 pixel buffer (RGBA8 packed、width * height * 4 bytes) — テスト用
    const gx::Vector<uint8_t>& GetPixels() const { return m_pixels; }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    gx::Vector<uint8_t> m_pixels;  ///< RGBA8 packed (4 bytes/cell)
    TextureManager* m_textureManager = nullptr;
    int m_textureHandle = -1;
    bool m_initialized = false;
    bool m_dirty = true;
    float m_worldExtent = k_DefaultWorldExtent;

    size_t PixelOffset(uint32_t x, uint32_t y) const
    {
        return (static_cast<size_t>(y) * m_width + x) * 4;
    }
};

} // namespace gx
/// @}
