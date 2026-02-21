#pragma once
/// @file Image.h
/// @brief 画像表示ウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief 画像のフィットモード
/// Stretch: 引き伸ばし、Contain: アスペクト比維持で内接、Cover: アスペクト比維持で外接（はみ出しはクリップ）
enum class ImageFit { Stretch, Contain, Cover };

/// @brief 画像表示ウィジェット
/// テクスチャハンドルを設定して画像を表示する。UV スクロールアニメーションにも対応。
class Image : public Widget
{
public:
    Image() = default;
    ~Image() override = default;

    WidgetType GetType() const override { return WidgetType::Image; }

    /// @brief 表示するテクスチャのハンドルを設定する
    /// @param handle TextureManagerで取得したハンドル
    void SetTextureHandle(int handle) { m_textureHandle = handle; }

    /// @brief テクスチャハンドルを取得する
    /// @return テクスチャハンドル
    int GetTextureHandle() const { return m_textureHandle; }

    /// @brief フィットモードを設定する
    /// @param fit フィットモード
    void SetFit(ImageFit fit) { m_fit = fit; }

    /// @brief フィットモードを取得する
    /// @return フィットモード
    ImageFit GetFit() const { return m_fit; }

    /// @brief 画像の元サイズを設定する（Contain/Coverのアスペクト比計算に使う）
    /// @param w 画像の自然な幅
    /// @param h 画像の自然な高さ
    void SetNaturalSize(float w, float h) { m_naturalWidth = w; m_naturalHeight = h; }

    /// @brief 画像の元の幅を取得する
    float GetNaturalWidth() const { return m_naturalWidth; }
    /// @brief 画像の元の高さを取得する
    float GetNaturalHeight() const { return m_naturalHeight; }

    float GetIntrinsicWidth() const override { return m_naturalWidth > 0 ? m_naturalWidth : 64.0f; }
    float GetIntrinsicHeight() const override { return m_naturalHeight > 0 ? m_naturalHeight : 64.0f; }

    void Update(float deltaTime) override;
    void RenderSelf(UIRenderer& renderer) override;

private:
    int m_textureHandle = -1;
    ImageFit m_fit = ImageFit::Stretch;
    float m_naturalWidth = 0.0f;
    float m_naturalHeight = 0.0f;
    float m_uvOffsetX = 0.0f;
    float m_uvOffsetY = 0.0f;
};

}} // namespace GX::GUI
