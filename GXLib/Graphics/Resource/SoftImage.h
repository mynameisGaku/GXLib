#pragma once
/// @file SoftImage.h
/// @brief CPU側での画像ピクセル操作
///
/// DxLibの MakeSoftImage() / GetPixelSoftImage() / DrawPixelSoftImage() に相当する。
/// GPUを介さずCPUメモリ上で画像のピクセルを直接読み書きできる。
///
/// 典型的な使い方:
///   1. Create() or LoadFromFile() で画像を用意
///   2. GetPixel() / DrawPixel() でピクセル単位の加工
///   3. CreateTexture() でGPUテクスチャに変換して描画に使う

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief CPUメモリ上の画像ピクセル操作クラス
///
/// 内部的にRGBA 4バイト/ピクセルで保持する。
/// プロシージャルテクスチャの生成やピクセル単位の画像解析に使える。
class SoftImage
{
public:
    SoftImage() = default;
    ~SoftImage() = default;

    /// @brief 空のSoftImageを作成する（全ピクセル0クリア）
    /// @param width 画像の幅（ピクセル）
    /// @param height 画像の高さ（ピクセル）
    /// @return 成功時true
    bool Create(uint32_t width, uint32_t height);

    /// @brief 画像ファイルから読み込む
    /// @param filePath 画像ファイルのパス（stb_image対応形式）
    /// @return 成功時true
    bool LoadFromFile(const std::wstring& filePath);

    /// @brief 指定座標のピクセル色を取得する
    /// @param x X座標
    /// @param y Y座標
    /// @return 0xAARRGGBB形式のカラー値。範囲外の場合は0
    uint32_t GetPixel(int x, int y) const;

    /// @brief 指定座標にピクセルを書き込む
    /// @param x X座標
    /// @param y Y座標
    /// @param color 0xAARRGGBB形式のカラー値
    void DrawPixel(int x, int y, uint32_t color);

    /// @brief CPUメモリの画像をGPUテクスチャとしてアップロードする
    /// @param textureManager テクスチャの作成先マネージャー
    /// @return テクスチャハンドル。失敗時は-1
    int CreateTexture(TextureManager& textureManager);

    /// @brief 全ピクセルを指定色でクリアする
    /// @param color 0xAARRGGBB形式のクリア色（デフォルトは透明黒）
    void Clear(uint32_t color = 0x00000000);

    /// @brief 画像の幅を取得する
    /// @return ピクセル単位の幅
    uint32_t GetWidth() const { return m_width; }

    /// @brief 画像の高さを取得する
    /// @return ピクセル単位の高さ
    uint32_t GetHeight() const { return m_height; }

    /// @brief 生のピクセルデータへのポインタを取得する
    /// @return RGBAピクセル配列の先頭ポインタ
    const uint8_t* GetPixels() const { return m_pixels.data(); }

private:
    std::vector<uint8_t> m_pixels; ///< RGBA 4バイト/ピクセルのピクセルデータ
    uint32_t m_width  = 0;
    uint32_t m_height = 0;
};

} // namespace GX
