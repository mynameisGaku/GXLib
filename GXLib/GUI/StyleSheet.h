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
    /// @brief CSSファイルからスタイルルールを読み込む
    /// @param path ファイルパス（VFS対応）
    /// @return 成功なら true
    bool LoadFromFile(const std::string& path);

    /// @brief CSS文字列からスタイルルールを読み込む
    /// @param source CSS形式の文字列
    /// @return 成功なら true
    bool LoadFromString(const std::string& source);

    /// @brief 単一ウィジェットにマッチするルールを適用する
    /// @param widget 適用対象のウィジェット
    void ApplyTo(Widget* widget) const;

    /// @brief ウィジェットツリー全体にスタイルを再帰適用する
    /// @param root ルートウィジェット
    void ApplyToTree(Widget* root) const;

    /// @brief 読み込み済みルール数を取得する
    /// @return ルール数
    size_t GetRuleCount() const { return m_rules.size(); }

    // --- Apply（GUILoaderのインラインスタイル用に公開） ---

    /// @brief プロパティ名をkebab-case("flex-direction")からcamelCase("flexDirection")に正規化する
    /// @param name 正規化前のプロパティ名
    /// @return 正規化後のプロパティ名
    static std::string NormalizePropertyName(const std::string& name);

    /// @brief プロパティ名と値の文字列からStyleフィールドに直接反映する
    /// @param style 反映先のスタイル
    /// @param name プロパティ名（kebab-case / camelCase 両対応）
    /// @param value プロパティ値の文字列
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
    static int ParseVec2(const std::string& value, float& outX, float& outY);
    static float ParseAngleDeg(const std::string& value);
    static float ParseRatio(const std::string& value);
    static UIEffectType ParseEffectType(const std::string& value);

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
