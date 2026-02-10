#pragma once
/// @file StyleSheet.h
/// @brief CSS スタイルシートパーサー
///
/// .css ファイルからスタイルルールを読み込み、ウィジェットツリーに適用する。
/// セレクタ（タイプ, .class, #id, :pseudo）によるカスケード解決を行う。

#include "pch.h"
#include "GUI/Style.h"

namespace GX { namespace GUI {

// 前方宣言
class Widget;
enum class WidgetType;

// ============================================================================
// 擬似クラス
// ============================================================================

enum class PseudoClass
{
    None,
    Hover,
    Pressed,
    Disabled,
    Focused
};

// ============================================================================
// セレクタ
// ============================================================================

/// @brief スタイルルールのセレクタ
struct StyleSelector
{
    std::string type;       ///< ウィジェットタイプ名 ("Panel", "Button" など)
    std::string className;  ///< クラス名 (先頭の . は含まない)
    std::string id;         ///< ID名 (先頭の # は含まない)
    PseudoClass pseudo = PseudoClass::None;

    /// ウィジェットにマッチするか判定
    bool Matches(const Widget* widget) const;

    /// 詳細度を計算 (id=100, class=10, type=1)
    int Specificity() const;

    /// セレクタ文字列をパース
    static StyleSelector Parse(const std::string& str);
};

// ============================================================================
// スタイルプロパティ
// ============================================================================

/// @brief 名前=値のペア（パース済みテキスト）
struct StyleProperty
{
    std::string name;
    std::string value;
};

// ============================================================================
// スタイルルール
// ============================================================================

/// @brief セレクタ + プロパティ群
struct StyleRule
{
    StyleSelector selector;
    int specificity = 0;
    int sourceOrder = 0;
    std::vector<StyleProperty> properties;
};

// ============================================================================
// スタイルシート
// ============================================================================

/// @brief CSS スタイルシートの読み込みと適用
class StyleSheet
{
public:
    /// ファイルからロード
    bool LoadFromFile(const std::string& path);

    /// 文字列からロード
    bool LoadFromString(const std::string& source);

    /// 単一ウィジェットにスタイルを適用
    void ApplyTo(Widget* widget) const;

    /// ウィジェットツリー全体に再帰適用
    void ApplyToTree(Widget* root) const;

    /// ルール数を取得
    size_t GetRuleCount() const { return m_rules.size(); }

    // --- Apply（GUILoaderのインラインスタイル用） ---
    static std::string NormalizePropertyName(const std::string& name);
    static void ApplyProperty(Style& style, const std::string& name, const std::string& value);

private:
    // --- トークナイザ ---
    enum class TokenType
    {
        Ident,      // 識別子
        Hash,       // #xxx
        Dot,        // .
        Colon,      // :
        LBrace,     // {
        RBrace,     // }
        Semicolon,  // ;
        Number,     // 数値 (負数含む)
        Percent,    // %
        String,     // "..." または '...'
        Eof
    };

    struct Token
    {
        TokenType type = TokenType::Eof;
        std::string text;
    };

    static std::vector<Token> Tokenize(const std::string& source);

    // --- パーサー ---
    void ParseTokens(const std::vector<Token>& tokens);
    static StyleSelector ParseSelector(const std::vector<Token>& tokens, size_t& pos);
    static std::vector<StyleProperty> ParsePropertyBlock(const std::vector<Token>& tokens, size_t& pos);

    // --- Apply（内部用） ---
    static StyleLength ParseLength(const std::string& value);
    static StyleColor ParseColor(const std::string& value);
    static StyleEdges ParseEdges(const std::string& value);

    // enum パーサー
    static FlexDirection    ParseFlexDirection(const std::string& v);
    static JustifyContent   ParseJustifyContent(const std::string& v);
    static AlignItems       ParseAlignItems(const std::string& v);
    static TextAlign        ParseTextAlign(const std::string& v);
    static VAlign           ParseVAlign(const std::string& v);
    static PositionType     ParsePosition(const std::string& v);
    static OverflowMode     ParseOverflow(const std::string& v);

    std::vector<StyleRule> m_rules;
};

// ============================================================================
// ユーティリティ
// ============================================================================

/// WidgetType を文字列に変換
const char* WidgetTypeToString(WidgetType type);

}} // namespace GX::GUI
