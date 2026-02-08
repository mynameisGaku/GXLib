#pragma once
/// @file FontManager.h
/// @brief フォントマネージャー — DirectWriteによるフォントラスタライズとテクスチャアトラス
///
/// 【初学者向け解説】
/// テキストを画面に描画するには、文字の形（グリフ）を画像に変換する必要があります。
/// DirectWriteはWindows標準のテキストレンダリングAPIで、高品質な文字描画が可能です。
///
/// FontManagerは以下の手順でテキスト描画を準備します：
/// 1. DirectWriteでフォントを作成
/// 2. 各文字（グリフ）をCPU側のビットマップにラスタライズ
/// 3. 全グリフを1枚の大きなテクスチャ（アトラス）にまとめる
/// 4. テクスチャアトラスをGPUにアップロード
/// 5. 各グリフのUV座標を記録
///
/// テクスチャアトラスとは、複数の小さな画像を1枚のテクスチャに配置する技術です。
/// テクスチャの切り替えはGPUにとってコストが高いため、アトラスにまとめることで
/// 効率的な描画が可能になります。

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief グリフ（1文字分）の描画情報
struct GlyphInfo
{
    float u0, v0;       ///< アトラス上の左上UV
    float u1, v1;       ///< アトラス上の右下UV
    int width;          ///< グリフの幅（ピクセル）
    int height;         ///< グリフの高さ（ピクセル）
    int offsetX;        ///< ベースラインからのX方向オフセット
    int offsetY;        ///< ベースラインからのY方向オフセット
    float advance;      ///< 次の文字への水平移動量
};

/// @brief フォントマネージャー
class FontManager
{
public:
    static constexpr uint32_t k_MaxFonts = 64;
    static constexpr uint32_t k_AtlasSize = 1024;  ///< アトラステクスチャサイズ

    FontManager() = default;
    ~FontManager() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, TextureManager* textureManager);

    /// フォントを作成してハンドルを返す
    int CreateFont(const std::wstring& fontName, int fontSize, bool bold = false, bool italic = false);

    /// グリフ情報を取得（未知文字は自動的にラスタライズ）
    const GlyphInfo* GetGlyphInfo(int fontHandle, wchar_t ch);

    /// フォントのアトラステクスチャハンドルを取得
    int GetAtlasTextureHandle(int fontHandle) const;

    /// フォントの行の高さを取得
    int GetLineHeight(int fontHandle) const;

    /// 終了処理
    void Shutdown();

private:
    /// フォントエントリ
    struct FontEntry
    {
        ComPtr<IDWriteTextFormat>  textFormat;
        std::unordered_map<wchar_t, GlyphInfo> glyphs;
        std::vector<uint32_t> atlasPixels;
        int atlasTextureHandle = -1;
        int fontSize = 0;
        int lineHeight = 0;
        int cursorX = 0;       ///< アトラスの現在の書き込みX位置
        int cursorY = 0;       ///< アトラスの現在の書き込みY位置
        int rowHeight = 0;     ///< 現在の行の最大高さ
        bool valid = false;
    };

    /// 1文字をラスタライズしてアトラスに追加
    bool RasterizeGlyph(FontEntry& entry, wchar_t ch);

    /// アトラステクスチャをGPUに（再）アップロード
    void UploadAtlas(FontEntry& entry);

    /// ASCII基本文字を一括ラスタライズ
    void RasterizeBasicChars(FontEntry& entry);

    int AllocateHandle();

    ID3D12Device*   m_device = nullptr;
    TextureManager* m_textureManager = nullptr;

    ComPtr<IDWriteFactory>   m_dwriteFactory;
    ComPtr<ID2D1Factory>     m_d2dFactory;

    std::vector<FontEntry> m_entries;
    std::vector<int>       m_freeHandles;
    int                    m_nextHandle = 0;
};

} // namespace GX
