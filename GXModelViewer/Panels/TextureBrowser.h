#pragma once
/// @file TextureBrowser.h
/// @brief グリッドサムネイル付きテクスチャブラウザパネル
///
/// TextureManagerに読み込まれた全テクスチャをサムネイルグリッドで表示し、
/// 選択するとサイズ・フォーマット・プレビューの詳細を確認できる。

#include "Graphics/Resource/TextureManager.h"

/// @brief 読み込み済みテクスチャの閲覧・検査パネル
class TextureBrowser
{
public:
    /// @brief テクスチャブラウザパネルを描画する
    /// @param texManager テクスチャ一覧の取得元
    void Draw(GX::TextureManager& texManager);

private:
    int m_selectedHandle = -1;      ///< 選択中のテクスチャハンドル（-1=未選択）
    float m_thumbnailSize = 64.0f;  ///< サムネイル表示サイズ（ピクセル）
};
