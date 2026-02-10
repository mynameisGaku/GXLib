#pragma once
/// @file GUILoader.h
/// @brief XML → ウィジェットツリー ローダー
///
/// XMLDocument からウィジェットツリーを構築し、
/// フォントハンドル、テクスチャハンドル、イベントハンドラの名前解決を行う。

#include "pch.h"
#include "GUI/XMLParser.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

// 前方宣言
class UIRenderer;

/// @brief XML からウィジェットツリーを構築するローダー
class GUILoader
{
public:
    /// UIRenderer を設定（TextWidget/Button の内容サイズ計算用）
    /// 初学者向け: 文字サイズから自然な幅・高さを決めるために使います。
    void SetRenderer(UIRenderer* renderer);

    /// フォント名 → フォントハンドルの登録
    void RegisterFont(const std::string& name, int fontHandle);

    /// イベント名 → ハンドラの登録
    void RegisterEvent(const std::string& name, std::function<void()> handler);

    /// テクスチャ名 → テクスチャハンドルの登録
    void RegisterTexture(const std::string& name, int textureHandle);

    /// 値変更イベント名 → ハンドラの登録
    void RegisterValueChangedEvent(const std::string& name,
                                    std::function<void(const std::string&)> handler);

    /// 描画コールバック名 → ハンドラの登録（Canvas用）
    void RegisterDrawCallback(const std::string& name,
                               std::function<void(UIRenderer&, const LayoutRect&)> handler);

    /// XML ファイルからウィジェットツリーを構築
    std::unique_ptr<Widget> BuildFromFile(const std::string& xmlPath);

    /// XMLDocument からウィジェットツリーを構築
    std::unique_ptr<Widget> BuildFromDocument(const XMLDocument& doc);

private:
    std::unique_ptr<Widget> BuildWidget(const XMLNode* node);
    int ResolveFontHandle(const std::string& fontName) const;
    int ResolveTextureHandle(const std::string& texName) const;

    UIRenderer* m_renderer = nullptr;
    std::unordered_map<std::string, int> m_fontMap;
    std::unordered_map<std::string, std::function<void()>> m_eventMap;
    std::unordered_map<std::string, int> m_textureMap;
    std::unordered_map<std::string, std::function<void(const std::string&)>> m_valueChangedMap;
    std::unordered_map<std::string, std::function<void(UIRenderer&, const LayoutRect&)>> m_drawCallbackMap;
};

}} // namespace GX::GUI
