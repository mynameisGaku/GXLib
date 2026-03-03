#pragma once
/// @file DropDown.h
/// @brief ドロップダウンウィジェット

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

class UIRenderer;

/// @brief ドロップダウン選択ウィジェット
/// ヘッダーをクリックするとポップアップリストが開き、項目を選択できる。
/// ポップアップは遅延描画(DeferDraw)で全ウィジェットの上に描画される。
class DropDown : public Widget
{
public:
    DropDown() = default;
    ~DropDown() override = default;

    WidgetType GetType() const override { return WidgetType::DropDown; }

    /// @brief 選択肢リストを設定する
    /// @param items 選択肢の文字列配列（UTF-8）
    void SetItems(const gx::Vector<gx::String>& items);

    /// @brief 選択中のインデックスを設定する
    /// @param index 選択インデックス
    void SetSelectedIndex(int index);

    /// @brief 選択中のインデックスを取得する
    /// @return 選択インデックス
    int GetSelectedIndex() const { return m_selectedIndex; }

    /// @brief ポップアップが開いているか
    /// @return 開いていれば true
    bool IsOpen() const { return m_open; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する（テキスト幅の計測に必要）
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 150.0f; }
    float GetIntrinsicHeight() const override { return 30.0f; }

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

private:
    static constexpr float k_ItemHeight = 28.0f;
    static constexpr float k_ArrowWidth = 20.0f;
    static constexpr float k_DropPadding = 4.0f;

    gx::Vector<gx::String> m_items;      ///< 選択肢リスト（UTF-8）
    gx::Vector<gx::WString> m_wideItems; ///< 選択肢リスト（ワイド文字）
    int m_selectedIndex = 0;               ///< 選択中インデックス
    int m_hoveredItem = -1;                ///< ホバー中インデックス
    bool m_open = false;                   ///< ポップアップ開閉状態
    int m_fontHandle = -1;                 ///< フォントハンドル
    UIRenderer* m_renderer = nullptr;      ///< GUI描画レンダラー
};

/// @}
}} // namespace gx::GUI
