#pragma once
/// @file StyleSheet.h
/// @brief CSS スタイルシートパーサー
///
/// .css ファイルからスタイルルールを読み込み、ウィジェットツリーに適用する。
/// セレクタ（タイプ, .class, #id, :pseudo）によるカスケード解決を行う。

#include "pch_graphics.h"
#include "GUI/Style.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui
/// @{

// 前方宣言
class Widget;
enum class WidgetType;

// ============================================================================
// 擬似クラス
// ============================================================================

/// @brief CSS擬似クラス
enum class PseudoClass
{
    None,      ///< 擬似クラスなし（通常状態）
    Hover,     ///< マウスホバー時 (:hover)
    Pressed,   ///< マウス押下時 (:pressed / :active)
    Disabled,  ///< 無効時 (:disabled)
    Focused    ///< フォーカス時 (:focus)
};

// ============================================================================
// セレクタ
// ============================================================================

/// @brief スタイルルールのセレクタ
struct StyleSelector
{
    gx::String type;       ///< ウィジェットタイプ名 ("Panel", "Button" など)
    gx::String className;  ///< クラス名 (先頭の . は含まない)
    gx::String id;         ///< ID名 (先頭の # は含まない)
    PseudoClass pseudo = PseudoClass::None; ///< 擬似クラス条件

    /// ウィジェットにマッチするか判定
    bool Matches(const Widget* widget) const;

    /// 詳細度を計算 (id=100, class=10, type=1)
    int Specificity() const;

    /// セレクタ文字列をパース
    static StyleSelector Parse(const gx::String& str);
};

// ============================================================================
// スタイルプロパティ
// ============================================================================

/// @brief 名前=値のペア（パース済みテキスト）
struct StyleProperty
{
    gx::String name;   ///< プロパティ名
    gx::String value;  ///< プロパティ値（文字列）
};

// ============================================================================
// スタイルルール
// ============================================================================

/// @brief セレクタ + プロパティ群
struct StyleRule
{
    StyleSelector selector;                ///< マッチ条件のセレクタ
    int specificity = 0;                   ///< 詳細度（id=100, class=10, type=1）
    int sourceOrder = 0;                   ///< ソース中の出現順（同一詳細度の優先度判定用）
    gx::Vector<StyleProperty> properties; ///< プロパティ配列
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
    bool LoadFromFile(const gx::String& path);

    /// @brief CSS文字列からスタイルルールを読み込む
    /// @param source CSS形式の文字列
    /// @return 成功なら true
    bool LoadFromString(const gx::String& source);

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
    static gx::String NormalizePropertyName(const gx::String& name);

    /// @brief プロパティ名と値の文字列からStyleフィールドに直接反映する
    /// @param style 反映先のスタイル
    /// @param name プロパティ名（kebab-case / camelCase 両対応）
    /// @param value プロパティ値の文字列
    static void ApplyProperty(Style& style, const gx::String& name, const gx::String& value);

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
        TokenType type = TokenType::Eof; ///< トークン種別
        gx::String text;                ///< トークンテキスト
    };

    static gx::Vector<Token> Tokenize(const gx::String& source);

    // --- パーサー ---
    void ParseTokens(const gx::Vector<Token>& tokens);
    static StyleSelector ParseSelector(const gx::Vector<Token>& tokens, size_t& pos);
    static gx::Vector<StyleProperty> ParsePropertyBlock(const gx::Vector<Token>& tokens, size_t& pos);

    // --- Apply（内部用） ---
    static StyleLength ParseLength(const gx::String& value);
    static StyleColor ParseColor(const gx::String& value);
    static StyleEdges ParseEdges(const gx::String& value);
    static int ParseVec2(const gx::String& value, float& outX, float& outY);
    static float ParseAngleDeg(const gx::String& value);
    static float ParseRatio(const gx::String& value);
    static UIEffectType ParseEffectType(const gx::String& value);

    // enum パーサー
    static FlexDirection    ParseFlexDirection(const gx::String& v);
    static JustifyContent   ParseJustifyContent(const gx::String& v);
    static AlignItems       ParseAlignItems(const gx::String& v);
    static TextAlign        ParseTextAlign(const gx::String& v);
    static VAlign           ParseVAlign(const gx::String& v);
    static PositionType     ParsePosition(const gx::String& v);
    static OverflowMode     ParseOverflow(const gx::String& v);

    gx::Vector<StyleRule> m_rules;  ///< パース済みスタイルルール配列
};

// ============================================================================
// ユーティリティ
// ============================================================================

/// WidgetType を文字列に変換
const char* WidgetTypeToString(WidgetType type);

/// @}
}} // namespace gx::GUI
