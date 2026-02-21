#pragma once
/// @file Spacer.h
/// @brief 空白スペーサーウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief 空白スペーサー（描画なし、レイアウト上の余白確保に使う）
/// SetSizeで指定した幅/高さ分のスペースを確保する。flexGrowと組み合わせると可変スペースにもなる。
class Spacer : public Widget
{
public:
    Spacer() = default;
    ~Spacer() override = default;

    WidgetType GetType() const override { return WidgetType::Spacer; }

    /// @brief スペーサーのサイズを設定する
    /// @param w 幅（ピクセル）
    /// @param h 高さ（ピクセル）
    void SetSize(float w, float h) { m_width = w; m_height = h; }

    float GetIntrinsicWidth() const override { return m_width; }
    float GetIntrinsicHeight() const override { return m_height; }

    void RenderSelf(UIRenderer& /*renderer*/) override {}

private:
    float m_width = 0.0f;
    float m_height = 0.0f;
};

}} // namespace GX::GUI
