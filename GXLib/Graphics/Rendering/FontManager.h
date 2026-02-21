#pragma once
/// @file FontManager.h
/// @brief フォントマネージャ -- DirectWriteによるフォント管理とテクスチャアトラス
///
/// DxLibの CreateFontToHandle / DeleteFontToHandle に相当するフォント管理を提供する。
/// 内部では DirectWrite でグリフ（文字の形）をラスタライズし、
/// 2048x2048 のテクスチャアトラスにまとめてGPUにアップロードする。
///
/// 漢字などはオンデマンドでラスタライズされるため、
/// 初回表示時にわずかなコストが発生するが、2回目以降はキャッシュから即座に描画される。
///
/// 処理の流れ:
///   1. CreateFont() で DirectWrite のフォントを作成
///   2. GetGlyphInfo() で文字ごとの描画情報を取得（未知の文字は自動ラスタライズ）
///   3. FlushAtlasUpdates() で変更があったアトラスをGPUに転送

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"
#include <wincodec.h>
#include <dwrite_1.h>

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

/// @brief フォントマネージャ（DxLibの CreateFontToHandle / DeleteFontToHandle に相当）
class FontManager
{
public:
    /// @brief 同時に管理できるフォントの最大数
    static constexpr uint32_t k_MaxFonts = 64;
    /// @brief アトラステクスチャのサイズ（ピクセル四方）
    static constexpr uint32_t k_AtlasSize = 2048;

    FontManager() = default;
    ~FontManager() = default;

    /// @brief フォントマネージャを初期化する
    /// @param device D3D12デバイスへのポインタ
    /// @param textureManager テクスチャマネージャへのポインタ（アトラスの作成・更新に使用）
    /// @return 初期化に成功した場合 true
    bool Initialize(ID3D12Device* device, TextureManager* textureManager);

    /// @brief フォントを作成してハンドルを返す
    ///
    /// 初期化時にASCII・ひらがな・カタカナ等の基本文字セットを事前ラスタライズする。
    /// 漢字など未登録の文字は GetGlyphInfo() 呼び出し時に自動追加される。
    ///
    /// @param fontName フォント名（例: L"MS Gothic", L"Yu Gothic UI"）
    /// @param fontSize フォントサイズ（ピクセル）
    /// @param bold 太字にするか
    /// @param italic 斜体にするか
    /// @return フォントハンドル（失敗時は -1）
    int CreateFont(const std::wstring& fontName, int fontSize, bool bold = false, bool italic = false);

    /// @brief 指定文字のグリフ情報を取得する（未知の文字は自動ラスタライズ）
    /// @param fontHandle CreateFont() で取得したフォントハンドル
    /// @param ch 取得する文字
    /// @return グリフ情報へのポインタ（失敗時は nullptr）
    const GlyphInfo* GetGlyphInfo(int fontHandle, wchar_t ch);

    /// @brief フォントのアトラステクスチャハンドルを取得する
    /// @param fontHandle フォントハンドル
    /// @return テクスチャハンドル（失敗時は -1）
    int GetAtlasTextureHandle(int fontHandle) const;

    /// @brief フォントの行の高さを取得する
    /// @param fontHandle フォントハンドル
    /// @return 行の高さ（ピクセル）
    int GetLineHeight(int fontHandle) const;

    /// @brief 上側の余白を詰めるための縦オフセットを取得する
    ///
    /// DirectWrite は ascent 基準でレイアウトするため、キャップハイト（大文字の高さ）との
    /// 差分だけ上に余白が出る。この値を差し引くとテキストが上に詰まって表示される。
    ///
    /// @param fontHandle フォントハンドル
    /// @return 縦オフセット（ピクセル）
    float GetCapOffset(int fontHandle) const;

    /// @brief 変更があったアトラスをGPUに転送する（毎フレーム末尾で呼ぶ）
    void FlushAtlasUpdates();

    /// @brief 終了処理を行い全リソースを解放する
    void Shutdown();

private:
    /// フォント1つ分のデータ
    struct FontEntry
    {
        ComPtr<IDWriteTextFormat>  textFormat;
        std::unordered_map<wchar_t, GlyphInfo> glyphs; ///< 文字→グリフ情報のキャッシュ
        std::vector<uint32_t> atlasPixels;              ///< CPU側のアトラスピクセルデータ (ARGB)
        int atlasTextureHandle = -1;  ///< GPU上のテクスチャハンドル
        int fontSize = 0;
        int lineHeight = 0;           ///< 行の高さ（ピクセル）
        float baseline = 0.0f;        ///< ベースラインまでの距離
        float capOffset = 0.0f;       ///< キャップハイト調整用オフセット
        int cursorX = 0;              ///< アトラスへの書き込み位置X
        int cursorY = 0;              ///< アトラスへの書き込み位置Y
        int rowHeight = 0;            ///< 現在の行の最大高さ
        bool valid = false;
        bool atlasDirty = false;      ///< CPUピクセルが更新されたがGPU未反映
    };

    /// 1文字をラスタライズしてアトラスに追加する
    bool RasterizeGlyph(FontEntry& entry, wchar_t ch);

    /// アトラスのピクセルデータをGPUテクスチャに転送する
    void UploadAtlas(FontEntry& entry);

    /// ASCII・ひらがな・カタカナ等の基本文字を一括ラスタライズする
    void RasterizeBasicChars(FontEntry& entry);

    /// フリーリストからハンドルを割り当てる
    int AllocateHandle();

    ID3D12Device*   m_device = nullptr;
    TextureManager* m_textureManager = nullptr;

    ComPtr<IDWriteFactory>      m_dwriteFactory;   ///< DirectWriteファクトリ
    ComPtr<ID2D1Factory>        m_d2dFactory;       ///< Direct2Dファクトリ（ラスタライズ用）
    ComPtr<IWICImagingFactory>  m_wicFactory;       ///< WICファクトリ（ビットマップ変換用）

    bool m_comInitialized = false;

    std::vector<FontEntry> m_entries;     ///< フォントエントリの配列
    std::vector<int>       m_freeHandles; ///< 解放済みハンドルのフリーリスト
    int                    m_nextHandle = 0;
};

} // namespace GX
