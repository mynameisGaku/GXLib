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
    /// @brief UIRendererを設定する（TextWidget/Buttonの内容サイズ計算に使う）
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer);

    /// @brief フォント名とフォントハンドルを対応付けて登録する
    /// @param name XMLの font 属性で指定する名前
    /// @param fontHandle FontManagerで取得したハンドル
    void RegisterFont(const std::string& name, int fontHandle);

    /// @brief イベント名とハンドラ関数を対応付けて登録する
    /// @param name XMLの onClick/onHover等で指定する名前
    /// @param handler 呼び出されるコールバック
    void RegisterEvent(const std::string& name, std::function<void()> handler);

    /// @brief テクスチャ名とテクスチャハンドルを対応付けて登録する
    /// @param name XMLの src 属性で指定する名前
    /// @param textureHandle TextureManagerで取得したハンドル
    void RegisterTexture(const std::string& name, int textureHandle);

    /// @brief 値変更イベント名とハンドラを対応付けて登録する（Slider/CheckBox/DropDown等用）
    /// @param name XMLの onValueChanged 属性で指定する名前
    /// @param handler 値変更時に呼ばれるコールバック（引数は値の文字列表現）
    void RegisterValueChangedEvent(const std::string& name,
                                    std::function<void(const std::string&)> handler);

    /// @brief 描画コールバック名とハンドラを対応付けて登録する（Canvas用）
    /// @param name XMLの onDraw 属性で指定する名前
    /// @param handler 描画時に呼ばれるコールバック
    void RegisterDrawCallback(const std::string& name,
                               std::function<void(UIRenderer&, const LayoutRect&)> handler);

    /// @brief XMLファイルからウィジェットツリーを構築する
    /// @param xmlPath XMLファイルのパス
    /// @return 構築されたルートウィジェット。失敗時は nullptr
    std::unique_ptr<Widget> BuildFromFile(const std::string& xmlPath);

    /// @brief パース済みXMLDocumentからウィジェットツリーを構築する
    /// @param doc XMLドキュメント
    /// @return 構築されたルートウィジェット。失敗時は nullptr
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
