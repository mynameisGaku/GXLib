#include "pch.h"
#include "GUI/StyleSheet.h"
#include "GUI/Widget.h"
#include "GUI/Widgets/Button.h"
#include "IO/FileSystem.h"

namespace GX { namespace GUI {

// ============================================================================
// ウィジェット種別 → 文字列
// ============================================================================

const char* WidgetTypeToString(WidgetType type)
{
    switch (type)
    {
    case WidgetType::Panel:       return "Panel";
    case WidgetType::Text:        return "Text";
    case WidgetType::Button:      return "Button";
    case WidgetType::Image:       return "Image";
    case WidgetType::TextInput:   return "TextInput";
    case WidgetType::Slider:      return "Slider";
    case WidgetType::CheckBox:    return "CheckBox";
    case WidgetType::RadioButton: return "RadioButton";
    case WidgetType::DropDown:    return "DropDown";
    case WidgetType::ListView:    return "ListView";
    case WidgetType::ScrollView:  return "ScrollView";
    case WidgetType::ProgressBar: return "ProgressBar";
    case WidgetType::TabView:     return "TabView";
    case WidgetType::Dialog:      return "Dialog";
    case WidgetType::Canvas:      return "Canvas";
    case WidgetType::Spacer:      return "Spacer";
    }
    return "Unknown";
}

// ============================================================================
// スタイルセレクタ
// ============================================================================

bool StyleSelector::Matches(const Widget* widget) const
{
    if (!widget) return false;

    // ID マッチ
    if (!id.empty() && widget->id != id) return false;

    // クラスマッチ
    if (!className.empty() && widget->className != className) return false;

    // タイプマッチ
    if (!type.empty())
    {
        const char* typeName = WidgetTypeToString(widget->GetType());
        if (type != typeName) return false;
    }

    // pseudo は Matches では無視（ApplyTo で振り分け）
    return true;
}

int StyleSelector::Specificity() const
{
    int s = 0;
    if (!id.empty())        s += 100;
    if (!className.empty()) s += 10;
    if (!type.empty())      s += 1;
    return s;
}

StyleSelector StyleSelector::Parse(const std::string& str)
{
    StyleSelector sel;
    size_t i = 0;

    // '#' id セレクタ
    if (i < str.size() && str[i] == '#')
    {
        ++i;
        size_t start = i;
        while (i < str.size() && str[i] != ':' && str[i] != '.' && str[i] != ' ')
            ++i;
        sel.id = str.substr(start, i - start);
    }

    // タイプセレクタ (英字で始まる)
    if (i < str.size() && std::isalpha(static_cast<unsigned char>(str[i])))
    {
        size_t start = i;
        while (i < str.size() && (std::isalnum(static_cast<unsigned char>(str[i])) || str[i] == '_'))
            ++i;
        sel.type = str.substr(start, i - start);
    }

    // '.' クラスセレクタ
    if (i < str.size() && str[i] == '.')
    {
        ++i;
        size_t start = i;
        while (i < str.size() && str[i] != ':' && str[i] != ' ')
            ++i;
        sel.className = str.substr(start, i - start);
    }

    // ':' 擬似クラス
    if (i < str.size() && str[i] == ':')
    {
        ++i;
        std::string pseudo = str.substr(i);
        if (pseudo == "hover")         sel.pseudo = PseudoClass::Hover;
        else if (pseudo == "pressed")  sel.pseudo = PseudoClass::Pressed;
        else if (pseudo == "disabled") sel.pseudo = PseudoClass::Disabled;
        else if (pseudo == "focused")  sel.pseudo = PseudoClass::Focused;
    }

    return sel;
}

// ============================================================================
// トークナイザ
// ============================================================================

std::vector<StyleSheet::Token> StyleSheet::Tokenize(const std::string& source)
{
    std::vector<Token> tokens;
    size_t i = 0;
    size_t len = source.size();

    while (i < len)
    {
        char c = source[i];

        // 空白スキップ
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            ++i;
            continue;
        }

        // ブロックコメント /* */
        if (c == '/' && i + 1 < len && source[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < len && !(source[i] == '*' && source[i + 1] == '/'))
                ++i;
            if (i + 1 < len) i += 2;
            continue;
        }

        // 行コメント //
        if (c == '/' && i + 1 < len && source[i + 1] == '/')
        {
            i += 2;
            while (i < len && source[i] != '\n')
                ++i;
            continue;
        }

        // 単一文字トークン
        if (c == '{')  { tokens.push_back({ TokenType::LBrace, "{" }); ++i; continue; }
        if (c == '}')  { tokens.push_back({ TokenType::RBrace, "}" }); ++i; continue; }
        if (c == ';')  { tokens.push_back({ TokenType::Semicolon, ";" }); ++i; continue; }
        if (c == '.')  { tokens.push_back({ TokenType::Dot, "." }); ++i; continue; }
        if (c == ':')  { tokens.push_back({ TokenType::Colon, ":" }); ++i; continue; }

        // #hash（色値またはid）
        if (c == '#')
        {
            ++i;
            size_t start = i;
            while (i < len && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
                ++i;
            tokens.push_back({ TokenType::Hash, source.substr(start, i - start) });
            continue;
        }

        // 文字列 "..." or '...'
        if (c == '"' || c == '\'')
        {
            char quote = c;
            ++i;
            size_t start = i;
            while (i < len && source[i] != quote)
                ++i;
            tokens.push_back({ TokenType::String, source.substr(start, i - start) });
            if (i < len) ++i;
            continue;
        }

        // 数値 (負数対応)
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '-' && i + 1 < len && std::isdigit(static_cast<unsigned char>(source[i + 1]))))
        {
            size_t start = i;
            if (c == '-') ++i;
            while (i < len && (std::isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.'))
                ++i;
            std::string numStr = source.substr(start, i - start);

            // % の場合
            if (i < len && source[i] == '%')
            {
                tokens.push_back({ TokenType::Percent, numStr });
                ++i;
            }
            else
            {
                tokens.push_back({ TokenType::Number, numStr });
            }
            continue;
        }

        // 識別子
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            size_t start = i;
            while (i < len && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_' || source[i] == '-'))
                ++i;
            tokens.push_back({ TokenType::Ident, source.substr(start, i - start) });
            continue;
        }

        // 不明文字をスキップ
        ++i;
    }

    tokens.push_back({ TokenType::Eof, "" });
    return tokens;
}

// ============================================================================
// パーサー
// ============================================================================

void StyleSheet::ParseTokens(const std::vector<Token>& tokens)
{
    m_rules.clear();
    size_t pos = 0;
    int orderCounter = 0;

    while (pos < tokens.size() && tokens[pos].type != TokenType::Eof)
    {
        // セレクタをパース
        StyleSelector sel = ParseSelector(tokens, pos);

        // '{' を期待
        if (pos >= tokens.size() || tokens[pos].type != TokenType::LBrace)
            break;
        ++pos; // skip '{'

        // プロパティブロックをパース
        auto props = ParsePropertyBlock(tokens, pos);

        // '}' をスキップ
        if (pos < tokens.size() && tokens[pos].type == TokenType::RBrace)
            ++pos;

        StyleRule rule;
        rule.selector = sel;
        rule.specificity = sel.Specificity();
        rule.sourceOrder = orderCounter++;
        rule.properties = std::move(props);
        m_rules.push_back(std::move(rule));
    }
}

StyleSelector StyleSheet::ParseSelector(const std::vector<Token>& tokens, size_t& pos)
{
    // セレクタ領域: '{' の前まで読み取ってセレクタ文字列を組み立てる
    std::string selectorStr;

    while (pos < tokens.size() && tokens[pos].type != TokenType::LBrace
                               && tokens[pos].type != TokenType::Eof)
    {
        auto& tok = tokens[pos];
        switch (tok.type)
        {
        case TokenType::Hash:      selectorStr += "#" + tok.text; break;
        case TokenType::Dot:       selectorStr += "."; break;
        case TokenType::Colon:     selectorStr += ":"; break;
        case TokenType::Ident:     selectorStr += tok.text; break;
        case TokenType::Number:    selectorStr += tok.text; break;
        default: break;
        }
        ++pos;
    }

    return StyleSelector::Parse(selectorStr);
}

std::vector<StyleProperty> StyleSheet::ParsePropertyBlock(const std::vector<Token>& tokens, size_t& pos)
{
    std::vector<StyleProperty> props;

    while (pos < tokens.size() && tokens[pos].type != TokenType::RBrace
                               && tokens[pos].type != TokenType::Eof)
    {
        // プロパティ名 (Ident)
        if (tokens[pos].type != TokenType::Ident)
        {
            ++pos;
            continue;
        }
        std::string name = tokens[pos].text;
        ++pos;

        // ':' をスキップ
        if (pos < tokens.size() && tokens[pos].type == TokenType::Colon)
            ++pos;

        // 値: ';' または '}' まで読み取る
        std::string value;
        while (pos < tokens.size() && tokens[pos].type != TokenType::Semicolon
                                   && tokens[pos].type != TokenType::RBrace
                                   && tokens[pos].type != TokenType::Eof)
        {
            auto& tok = tokens[pos];
            if (!value.empty()) value += " ";
            switch (tok.type)
            {
            case TokenType::Hash:    value += "#" + tok.text; break;
            case TokenType::Number:  value += tok.text; break;
            case TokenType::Percent: value += tok.text + "%"; break;
            case TokenType::Ident:   value += tok.text; break;
            case TokenType::String:  value += tok.text; break;
            default:                 value += tok.text; break;
            }
            ++pos;
        }

        // ';' をスキップ
        if (pos < tokens.size() && tokens[pos].type == TokenType::Semicolon)
            ++pos;

        props.push_back({ name, value });
    }

    return props;
}

// ============================================================================
// LoadFromFile / LoadFromString（読み込み）
// ============================================================================

bool StyleSheet::LoadFromFile(const std::string& path)
{
    auto fileData = GX::FileSystem::Instance().ReadFile(path);
    if (!fileData.IsValid())
    {
        // 直接ファイルI/Oにフォールバック
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::string source((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return LoadFromString(source);
    }
    return LoadFromString(fileData.AsString());
}

bool StyleSheet::LoadFromString(const std::string& source)
{
    auto tokens = Tokenize(source);
    ParseTokens(tokens);
    return !m_rules.empty();
}

// ============================================================================
// kebab-case → camelCase の正規化
// 例: "flex-direction" → "flexDirection", "border-radius" → "borderRadius"
// ============================================================================

std::string StyleSheet::NormalizePropertyName(const std::string& name)
{
    // CSS標準エイリアス
    if (name == "border-radius") return "cornerRadius";
    if (name == "background-color") return "backgroundColor";
    if (name == "transition-duration") return "transitionDuration";

    std::string result;
    result.reserve(name.size());
    bool nextUpper = false;
    for (char c : name)
    {
        if (c == '-')
        {
            nextUpper = true;
        }
        else
        {
            if (nextUpper)
            {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                nextUpper = false;
            }
            else
            {
                result += c;
            }
        }
    }
    return result;
}

// ============================================================================
// ApplyProperty: プロパティ名→Styleフィールドへの反映
// kebab-case / camelCase 両対応（正規化してからマッチ）
// ============================================================================

void StyleSheet::ApplyProperty(Style& style, const std::string& rawName, const std::string& value)
{
    std::string name = NormalizePropertyName(rawName);

    // --- サイズ ---
    if (name == "width")           { style.width    = ParseLength(value); return; }
    if (name == "height")          { style.height   = ParseLength(value); return; }
    if (name == "minWidth")        { style.minWidth  = ParseLength(value); return; }
    if (name == "minHeight")       { style.minHeight = ParseLength(value); return; }
    if (name == "maxWidth")        { style.maxWidth  = ParseLength(value); return; }
    if (name == "maxHeight")       { style.maxHeight = ParseLength(value); return; }

    // --- ボックスモデル ---
    if (name == "margin")          { style.margin  = ParseEdges(value); return; }
    if (name == "marginTop")       { style.margin.top    = std::stof(value); return; }
    if (name == "marginRight")     { style.margin.right  = std::stof(value); return; }
    if (name == "marginBottom")    { style.margin.bottom = std::stof(value); return; }
    if (name == "marginLeft")      { style.margin.left   = std::stof(value); return; }
    if (name == "padding")         { style.padding = ParseEdges(value); return; }
    if (name == "paddingTop")      { style.padding.top    = std::stof(value); return; }
    if (name == "paddingRight")    { style.padding.right  = std::stof(value); return; }
    if (name == "paddingBottom")   { style.padding.bottom = std::stof(value); return; }
    if (name == "paddingLeft")     { style.padding.left   = std::stof(value); return; }
    if (name == "borderWidth")     { style.borderWidth = std::stof(value); return; }
    if (name == "borderColor")     { style.borderColor = ParseColor(value); return; }

    // --- 背景 ---
    if (name == "background" || name == "backgroundColor")
    {
        style.backgroundColor = ParseColor(value);
        return;
    }
    if (name == "cornerRadius")    { style.cornerRadius = std::stof(value); return; }

    // --- テキスト ---
    if (name == "color")           { style.color    = ParseColor(value); return; }
    if (name == "fontSize")        { style.fontSize = std::stof(value); return; }
    if (name == "fontFamily")      { style.fontFamily = value; return; }
    if (name == "textAlign")       { style.textAlign = ParseTextAlign(value); return; }
    if (name == "verticalAlign")   { style.verticalAlign = ParseVAlign(value); return; }

    // --- Flexbox ---
    if (name == "flexDirection")   { style.flexDirection   = ParseFlexDirection(value); return; }
    if (name == "justifyContent")  { style.justifyContent  = ParseJustifyContent(value); return; }
    if (name == "alignItems")      { style.alignItems      = ParseAlignItems(value); return; }
    if (name == "flexGrow")        { style.flexGrow   = std::stof(value); return; }
    if (name == "flexShrink")      { style.flexShrink = std::stof(value); return; }
    if (name == "gap")             { style.gap = std::stof(value); return; }

    // --- 位置 ---
    if (name == "position")        { style.position = ParsePosition(value); return; }
    if (name == "left")            { style.posLeft = ParseLength(value); return; }
    if (name == "top")             { style.posTop  = ParseLength(value); return; }

    // --- オーバーフロー ---
    if (name == "overflow")        { style.overflow = ParseOverflow(value); return; }

    // --- 影 ---
    if (name == "shadowOffsetX")   { style.shadowOffsetX = std::stof(value); return; }
    if (name == "shadowOffsetY")   { style.shadowOffsetY = std::stof(value); return; }
    if (name == "shadowBlur")      { style.shadowBlur    = std::stof(value); return; }
    if (name == "shadowColor")     { style.shadowColor   = ParseColor(value); return; }

    // --- Opacity / Transform ---
    if (name == "opacity")         { style.opacity = std::stof(value); return; }
    if (name == "translateX")      { style.translateX = std::stof(value); return; }
    if (name == "translateY")      { style.translateY = std::stof(value); return; }
    if (name == "translate")
    {
        float x = 0.0f, y = 0.0f;
        int n = ParseVec2(value, x, y);
        style.translateX = x;
        style.translateY = (n >= 2) ? y : 0.0f;
        return;
    }
    if (name == "scaleX")          { style.scaleX = std::stof(value); return; }
    if (name == "scaleY")          { style.scaleY = std::stof(value); return; }
    if (name == "scale")
    {
        float x = 1.0f, y = 1.0f;
        int n = ParseVec2(value, x, y);
        style.scaleX = x;
        style.scaleY = (n >= 2) ? y : x;
        return;
    }
    if (name == "rotate")          { style.rotate = ParseAngleDeg(value); return; }
    if (name == "pivotX")          { style.pivotX = ParseRatio(value); return; }
    if (name == "pivotY")          { style.pivotY = ParseRatio(value); return; }
    if (name == "pivot")
    {
        float x = 0.5f, y = 0.5f;
        int n = ParseVec2(value, x, y);
        style.pivotX = (n >= 1) ? x : 0.5f;
        style.pivotY = (n >= 2) ? y : style.pivotX;
        return;
    }

    // --- Effects ---
    if (name == "effect")          { style.effectType = ParseEffectType(value); return; }
    if (name == "effectStrength")  { style.effectStrength = std::stof(value); return; }
    if (name == "effectWidth")     { style.effectWidth = std::stof(value); return; }
    if (name == "effectDuration")  { style.effectDuration = std::stof(value); return; }

    // --- Image UV ---
    if (name == "imageUvScaleX")   { style.imageUVScaleX = std::stof(value); return; }
    if (name == "imageUvScaleY")   { style.imageUVScaleY = std::stof(value); return; }
    if (name == "imageUvScale")
    {
        float x = 1.0f, y = 1.0f;
        int n = ParseVec2(value, x, y);
        style.imageUVScaleX = x;
        style.imageUVScaleY = (n >= 2) ? y : x;
        return;
    }
    if (name == "imageUvSpeedX")   { style.imageUVSpeedX = std::stof(value); return; }
    if (name == "imageUvSpeedY")   { style.imageUVSpeedY = std::stof(value); return; }
    if (name == "imageUvSpeed")
    {
        float x = 0.0f, y = 0.0f;
        int n = ParseVec2(value, x, y);
        style.imageUVSpeedX = x;
        style.imageUVSpeedY = (n >= 2) ? y : x;
        return;
    }

    // --- アニメーション ---
    if (name == "transitionDuration") { style.transitionDuration = std::stof(value); return; }
}

// ============================================================================
// 値パーサー
// ============================================================================

StyleLength StyleSheet::ParseLength(const std::string& value)
{
    if (value == "auto") return StyleLength::Auto();

    // パーセント
    if (!value.empty() && value.back() == '%')
    {
        try { return StyleLength::Pct(std::stof(value.substr(0, value.size() - 1))); }
        catch (...) { return StyleLength::Pct(0.0f); }
    }

    // px (数値のみ = px扱い)
    std::string numPart = value;
    // "100px" → "100"
    if (numPart.size() > 2 && numPart.substr(numPart.size() - 2) == "px")
        numPart = numPart.substr(0, numPart.size() - 2);

    try { return StyleLength::Px(std::stof(numPart)); } catch (...) { return StyleLength::Px(0.0f); }
}

StyleColor StyleSheet::ParseColor(const std::string& value)
{
    if (!value.empty() && value[0] == '#')
        return StyleColor::FromHex(value);

    // 名前付き色（最小限）
    if (value == "white")       return { 1.0f, 1.0f, 1.0f, 1.0f };
    if (value == "black")       return { 0.0f, 0.0f, 0.0f, 1.0f };
    if (value == "red")         return { 1.0f, 0.0f, 0.0f, 1.0f };
    if (value == "green")       return { 0.0f, 1.0f, 0.0f, 1.0f };
    if (value == "blue")        return { 0.0f, 0.0f, 1.0f, 1.0f };
    if (value == "transparent") return { 0.0f, 0.0f, 0.0f, 0.0f };

    return {};
}

StyleEdges StyleSheet::ParseEdges(const std::string& value)
{
    // 空白区切りで1〜4値
    std::vector<float> values;
    size_t i = 0;
    size_t len = value.size();
    while (i < len)
    {
        // 空白スキップ
        while (i < len && std::isspace(static_cast<unsigned char>(value[i]))) ++i;
        if (i >= len) break;

        // 数値の開始位置
        size_t start = i;
        if (value[i] == '-') ++i;
        while (i < len && (std::isdigit(static_cast<unsigned char>(value[i])) || value[i] == '.'))
            ++i;
        if (i > start)
            values.push_back(std::stof(value.substr(start, i - start)));
    }

    if (values.size() == 1)
        return StyleEdges(values[0]);
    if (values.size() == 2)
        return StyleEdges(values[0], values[1]);
    if (values.size() >= 4)
        return StyleEdges(values[0], values[1], values[2], values[3]);

    return {};
}

int StyleSheet::ParseVec2(const std::string& value, float& outX, float& outY)
{
    outX = 0.0f;
    outY = 0.0f;
    std::vector<float> values;
    size_t i = 0;
    size_t len = value.size();
    while (i < len)
    {
        while (i < len && std::isspace(static_cast<unsigned char>(value[i]))) ++i;
        if (i >= len) break;

        size_t start = i;
        if (value[i] == '-') ++i;
        while (i < len && (std::isdigit(static_cast<unsigned char>(value[i])) || value[i] == '.'))
            ++i;
        if (i > start)
            values.push_back(std::stof(value.substr(start, i - start)));
        else
            ++i;
    }

    if (values.size() >= 1) outX = values[0];
    if (values.size() >= 2) outY = values[1];
    return static_cast<int>(values.size());
}

float StyleSheet::ParseAngleDeg(const std::string& value)
{
    if (value.size() > 3 && value.substr(value.size() - 3) == "deg")
        return std::stof(value.substr(0, value.size() - 3));
    if (value.size() > 3 && value.substr(value.size() - 3) == "rad")
        return std::stof(value.substr(0, value.size() - 3)) * (180.0f / 3.1415926535f);
    return std::stof(value);
}

float StyleSheet::ParseRatio(const std::string& value)
{
    if (!value.empty() && value.back() == '%')
    {
        float v = std::stof(value.substr(0, value.size() - 1));
        return v * 0.01f;
    }
    return std::stof(value);
}

UIEffectType StyleSheet::ParseEffectType(const std::string& value)
{
    if (value == "ripple") return UIEffectType::Ripple;
    return UIEffectType::None;
}

// ============================================================================
// enum パーサー
// ============================================================================

FlexDirection StyleSheet::ParseFlexDirection(const std::string& v)
{
    if (v == "row") return FlexDirection::Row;
    return FlexDirection::Column; // default
}

JustifyContent StyleSheet::ParseJustifyContent(const std::string& v)
{
    if (v == "center")        return JustifyContent::Center;
    if (v == "end")           return JustifyContent::End;
    if (v == "space-between") return JustifyContent::SpaceBetween;
    if (v == "spaceBetween")  return JustifyContent::SpaceBetween;
    if (v == "space-around")  return JustifyContent::SpaceAround;
    if (v == "spaceAround")   return JustifyContent::SpaceAround;
    return JustifyContent::Start;
}

AlignItems StyleSheet::ParseAlignItems(const std::string& v)
{
    if (v == "center")  return AlignItems::Center;
    if (v == "end")     return AlignItems::End;
    if (v == "stretch") return AlignItems::Stretch;
    return AlignItems::Start;
}

TextAlign StyleSheet::ParseTextAlign(const std::string& v)
{
    if (v == "center") return TextAlign::Center;
    if (v == "right")  return TextAlign::Right;
    return TextAlign::Left;
}

VAlign StyleSheet::ParseVAlign(const std::string& v)
{
    if (v == "center") return VAlign::Center;
    if (v == "bottom") return VAlign::Bottom;
    return VAlign::Top;
}

PositionType StyleSheet::ParsePosition(const std::string& v)
{
    if (v == "absolute") return PositionType::Absolute;
    return PositionType::Relative;
}

OverflowMode StyleSheet::ParseOverflow(const std::string& v)
{
    if (v == "hidden") return OverflowMode::Hidden;
    if (v == "scroll") return OverflowMode::Scroll;
    return OverflowMode::Visible;
}

// ============================================================================
// カスケード適用
// ============================================================================

void StyleSheet::ApplyTo(Widget* widget) const
{
    if (!widget) return;

    // マッチするルールを収集
    struct MatchedRule
    {
        const StyleRule* rule;
        int specificity;
        int sourceOrder;
    };
    std::vector<MatchedRule> matched;

    for (const auto& rule : m_rules)
    {
        if (rule.selector.Matches(widget))
            matched.push_back({ &rule, rule.specificity, rule.sourceOrder });
    }

    // specificity 昇順 → sourceOrder 昇順でソート（後のルールが優先）
    std::sort(matched.begin(), matched.end(), [](const MatchedRule& a, const MatchedRule& b)
    {
        if (a.specificity != b.specificity) return a.specificity < b.specificity;
        return a.sourceOrder < b.sourceOrder;
    });

    // baseStyle に pseudo=None のルールを順に適用
    Style baseStyle;
    for (const auto& m : matched)
    {
        if (m.rule->selector.pseudo == PseudoClass::None)
        {
            for (const auto& prop : m.rule->properties)
                ApplyProperty(baseStyle, prop.name, prop.value);
        }
    }

    // 擬似クラス（hover/pressed/disabled/focused）を現在状態に適用
    Style currentStyle = baseStyle;
    for (const auto& m : matched)
    {
        bool apply = false;
        switch (m.rule->selector.pseudo)
        {
        case PseudoClass::Hover:
            apply = widget->hovered;
            break;
        case PseudoClass::Pressed:
            apply = widget->pressed;
            break;
        case PseudoClass::Disabled:
            apply = !widget->enabled;
            break;
        case PseudoClass::Focused:
            apply = widget->focused;
            break;
        default:
            break;
        }
        if (apply)
        {
            for (const auto& prop : m.rule->properties)
                ApplyProperty(currentStyle, prop.name, prop.value);
        }
    }
    widget->computedStyle = currentStyle;
}

void StyleSheet::ApplyToTree(Widget* root) const
{
    if (!root) return;

    ApplyTo(root);

    for (auto& child : root->GetChildren())
        ApplyToTree(child.get());
}

}} // namespace GX::GUI
