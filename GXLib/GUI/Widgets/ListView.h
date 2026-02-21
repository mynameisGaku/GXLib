#pragma once
/// @file ListView.h
/// @brief リストビューウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief リストビューウィジェット（スクロール可能なアイテム選択リスト）
/// 文字列リストを表示し、クリックでアイテムを選択できる。
/// マウスホイールでスクロールし、スクロールバーも自動表示される。
class ListView : public Widget
{
public:
    ListView() = default;
    ~ListView() override = default;

    WidgetType GetType() const override { return WidgetType::ListView; }

    /// @brief アイテムリストを設定する（設定すると選択がリセットされる）
    /// @param items アイテムの文字列配列（UTF-8）
    void SetItems(const std::vector<std::string>& items);

    /// @brief 選択中のインデックスを設定する
    /// @param index 選択インデックス（-1で未選択）
    void SetSelectedIndex(int index);

    /// @brief 選択中のインデックスを取得する
    /// @return 選択インデックス（-1=未選択）
    int GetSelectedIndex() const { return m_selectedIndex; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 150.0f; }

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

private:
    void ClampScroll();

    static constexpr float k_ItemHeight = 28.0f;
    static constexpr float k_ScrollbarWidth = 4.0f;

    std::vector<std::string> m_items;
    std::vector<std::wstring> m_wideItems;
    int m_selectedIndex = -1;
    int m_hoveredItem = -1;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
