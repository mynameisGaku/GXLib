#pragma once
/// @file TabView.h
/// @brief タブビューウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief タブビューウィジェット
/// 上部にタブヘッダーを表示し、子ウィジェットをタブコンテンツとして切り替え表示する。
/// 子ウィジェットの順番がタブの順番に対応する（0番目の子=0番目のタブのコンテンツ）。
class TabView : public Widget
{
public:
    TabView() = default;
    ~TabView() override = default;

    WidgetType GetType() const override { return WidgetType::TabView; }

    /// @brief タブ名のリストを設定する
    /// @param names タブ名の文字列配列（UTF-8）
    void SetTabNames(const std::vector<std::string>& names);

    /// @brief アクティブタブを切り替える
    /// @param index タブインデックス
    void SetActiveTab(int index);

    /// @brief アクティブタブのインデックスを取得する
    /// @return アクティブタブのインデックス
    int GetActiveTab() const { return m_activeTab; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 300.0f; }
    float GetIntrinsicHeight() const override { return 200.0f; }

    bool OnEvent(const UIEvent& event) override;
    void Update(float deltaTime) override;
    void RenderSelf(UIRenderer& renderer) override;
    void RenderChildren(UIRenderer& renderer) override;

private:
    static constexpr float k_TabHeaderHeight = 32.0f;
    static constexpr float k_TabPadding = 12.0f;

    std::vector<std::string> m_tabNames;
    std::vector<std::wstring> m_wideTabNames;
    int m_activeTab = 0;
    int m_hoveredTab = -1;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
