#pragma once
/// @file AtlasPacker.h
/// @brief テクスチャアトラスパッキング（MaxRects BSSF アルゴリズム）
///
/// 複数の矩形（画像）を1枚のテクスチャアトラスに自動配置する。
/// FontManager や SpriteSheet のアトラス生成など、
/// 多数の小画像をまとめて1枚のテクスチャに詰め込む場面で使う。
/// 出力は Power-of-2 サイズに丸められ、失敗時は success==false になる。
/// @addtogroup grp_graphics/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief アトラスパッキングの入力（名前付き矩形）
struct AtlasInput
{
    std::string name;
    uint32_t width;
    uint32_t height;
};

/// @brief アトラスパッキング結果の1エントリ
struct AtlasEntry
{
    std::string name;
    uint32_t x, y;
    uint32_t width, height;
};

/// @brief アトラスパッキング結果
struct AtlasResult
{
    uint32_t atlasWidth  = 0;
    uint32_t atlasHeight = 0;
    std::vector<AtlasEntry> entries;
    bool success = false;
};

/// @brief テクスチャアトラスパッカー（MaxRects Best Short Side Fit）
class AtlasPacker
{
public:
    /// @brief 最大アトラスサイズを設定する
    /// @param maxWidth 最大幅
    /// @param maxHeight 最大高さ
    void SetMaxSize(uint32_t maxWidth, uint32_t maxHeight);

    /// @brief パディング（ブリード防止用の余白）を設定する
    /// @param padding パディング量（ピクセル）
    void SetPadding(uint32_t padding);

    /// @brief 矩形をパッキングする
    /// @param inputs 入力矩形の配列
    /// @return パッキング結果
    AtlasResult Pack(const std::vector<AtlasInput>& inputs);

private:
    struct Rect
    {
        uint32_t x, y, width, height;
    };

    bool TryPack(const std::vector<AtlasInput>& sorted, uint32_t atlasW, uint32_t atlasH,
                 std::vector<AtlasEntry>& outEntries);
    Rect FindBestBSSF(const std::vector<Rect>& freeRects, uint32_t w, uint32_t h, int& bestScore);
    void SplitFreeRects(std::vector<Rect>& freeRects, const Rect& placed);
    void PruneFreeRects(std::vector<Rect>& freeRects);

    static uint32_t NextPowerOf2(uint32_t v);

    uint32_t m_maxWidth  = 4096;
    uint32_t m_maxHeight = 4096;
    uint32_t m_padding   = 0;
};

} // namespace gx
/// @}
