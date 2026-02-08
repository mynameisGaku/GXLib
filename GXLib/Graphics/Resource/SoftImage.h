#pragma once
/// @file SoftImage.h
/// @brief ソフトウェアイメージ — CPUメモリ上のピクセル操作
///
/// 【初学者向け解説】
/// SoftImageは、CPUメモリ上で画像のピクセルを直接読み書きするためのクラスです。
/// DXLibのLoadSoftImage/GetPixelSoftImage/DrawPixelSoftImageに相当します。
///
/// GPUテクスチャへの変換も可能なので：
/// 1. SoftImageで画像を生成・加工
/// 2. CreateTexture()でGPUに転送
/// 3. SpriteBatchで描画
/// という流れで使えます。

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief CPUメモリ上のピクセル操作クラス
class SoftImage
{
public:
    SoftImage() = default;
    ~SoftImage() = default;

    /// 空のSoftImageを作成
    bool Create(uint32_t width, uint32_t height);

    /// ファイルから読み込み
    bool LoadFromFile(const std::wstring& filePath);

    /// ピクセルを取得（RGBA）
    /// @return 0xAARRGGBB形式のカラー値
    uint32_t GetPixel(int x, int y) const;

    /// ピクセルを書き込み
    /// @param color 0xAARRGGBB形式のカラー値
    void DrawPixel(int x, int y, uint32_t color);

    /// テクスチャとしてGPUにアップロード
    /// @return テクスチャハンドル（失敗時は-1）
    int CreateTexture(TextureManager& textureManager);

    /// 画像全体をクリア
    void Clear(uint32_t color = 0x00000000);

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const uint8_t* GetPixels() const { return m_pixels.data(); }

private:
    std::vector<uint8_t> m_pixels;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;
};

} // namespace GX
