#pragma once
/// @file TabView.h
/// @brief タブビューウィジェット

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

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

    std::vector<std::string> m_tabNames;      ///< タブ名リスト（UTF-8）
    std::vector<std::wstring> m_wideTabNames; ///< タブ名リスト（ワイド文字）
    int m_activeTab = 0;                      ///< アクティブタブインデックス
    int m_hoveredTab = -1;                    ///< ホバー中タブインデックス
    int m_fontHandle = -1;                    ///< フォントハンドル
    UIRenderer* m_renderer = nullptr;         ///< GUI描画レンダラー
};

/// @}
}} // namespace gx::GUI
