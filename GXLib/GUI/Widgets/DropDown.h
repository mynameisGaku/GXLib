#pragma once
/// @file DropDown.h
/// @brief ドロップダウンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

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
    void SetItems(const std::vector<std::string>& items);

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

    std::vector<std::string> m_items;
    std::vector<std::wstring> m_wideItems;
    int m_selectedIndex = 0;
    int m_hoveredItem = -1;
    bool m_open = false;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
