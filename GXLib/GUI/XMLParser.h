#pragma once
/// @file XMLParser.h
/// @brief XML DOM パーサー
///
/// XML ファイルからウィジェットツリーを構築するための軽量 XML パーサー。
/// 再帰降下パーサーで XML DOM を構築する。

#include "pch_graphics.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui
/// @{

// ============================================================================
// XMLNode（XMLノード）
// ============================================================================

/// @brief XML DOM ノード
struct XMLNode
{
    std::string tag;        ///< タグ名 ("Panel", "Text", "Button")
    std::string text;       ///< テキストコンテント
    std::unordered_map<std::string, std::string> attributes; ///< 属性名→値マップ
    std::vector<std::unique_ptr<XMLNode>> children;        ///< 子ノード配列

    /// 属性値を取得（存在しない場合は defaultVal を返す）
    const std::string& GetAttribute(const std::string& name, const std::string& defaultVal = "") const
    {
        auto it = attributes.find(name);
        if (it != attributes.end()) return it->second;
        return defaultVal;
    }

    /// 属性が存在するか
    bool HasAttribute(const std::string& name) const
    {
        return attributes.find(name) != attributes.end();
    }
};

// ============================================================================
// XMLDocument（XMLドキュメント）
// ============================================================================

/// @brief XML ドキュメント（DOM ルート）
class XMLDocument
{
public:
    /// @brief XMLファイルからDOMツリーを読み込む
    /// @param path ファイルパス（VFS対応）
    /// @return 成功なら true
    bool LoadFromFile(const std::string& path);

    /// @brief XML文字列からDOMツリーを読み込む
    /// @param source XML形式の文字列
    /// @return 成功なら true
    bool LoadFromString(const std::string& source);

    /// @brief ルートノードを取得する
    /// @return ルートXMLノード。読み込み前は nullptr
    XMLNode* GetRoot() const { return m_root.get(); }

private:
    struct ParseContext
    {
        const std::string& source;  ///< パース対象のXML文字列
        size_t pos = 0;             ///< 現在の読み取り位置
    };

    std::unique_ptr<XMLNode> ParseNode(ParseContext& ctx);
    std::string ParseTagName(ParseContext& ctx);
    void ParseAttributes(ParseContext& ctx, std::unordered_map<std::string, std::string>& attrs);
    std::string ParseAttributeValue(ParseContext& ctx);
    std::string ParseTextContent(ParseContext& ctx);
    void SkipWhitespace(ParseContext& ctx);
    bool SkipComment(ParseContext& ctx);
    void SkipXMLDeclaration(ParseContext& ctx);
    static std::string DecodeEntities(const std::string& text);

    std::unique_ptr<XMLNode> m_root;  ///< ルートXMLノード
};

/// @}
}} // namespace gx::GUI
