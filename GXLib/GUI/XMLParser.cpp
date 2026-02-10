#include "pch.h"
#include "GUI/XMLParser.h"
#include "IO/FileSystem.h"
#include "Core/Logger.h"

namespace GX { namespace GUI {

// ============================================================================
// LoadFromFile（ファイル読み込み）
// ============================================================================

bool XMLDocument::LoadFromFile(const std::string& path)
{
    auto fileData = GX::FileSystem::Instance().ReadFile(path);
    if (!fileData.IsValid())
    {
        // 直接ファイルI/Oにフォールバック
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            GX_LOG_ERROR("XMLDocument: Failed to open file: %s", path.c_str());
            return false;
        }
        std::string source((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return LoadFromString(source);
    }
    return LoadFromString(fileData.AsString());
}

// ============================================================================
// LoadFromString（文字列読み込み）
// ============================================================================

bool XMLDocument::LoadFromString(const std::string& source)
{
    ParseContext ctx{ source, 0 };

    // UTF-8 BOM スキップ
    if (source.size() >= 3 &&
        static_cast<unsigned char>(source[0]) == 0xEF &&
        static_cast<unsigned char>(source[1]) == 0xBB &&
        static_cast<unsigned char>(source[2]) == 0xBF)
    {
        ctx.pos = 3;
    }

    // XML宣言・コメントをスキップ
    SkipWhitespace(ctx);
    SkipXMLDeclaration(ctx);

    // ルートノードをパース
    SkipWhitespace(ctx);
    while (SkipComment(ctx))
        SkipWhitespace(ctx);

    m_root = ParseNode(ctx);
    if (!m_root)
    {
        GX_LOG_ERROR("XMLDocument: Failed to parse root node");
        return false;
    }

    return true;
}

// ============================================================================
// ParseNode（ノード解析）
// ============================================================================

std::unique_ptr<XMLNode> XMLDocument::ParseNode(ParseContext& ctx)
{
    SkipWhitespace(ctx);
    while (SkipComment(ctx))
        SkipWhitespace(ctx);

    if (ctx.pos >= ctx.source.size() || ctx.source[ctx.pos] != '<')
        return nullptr;

    // '</...' なら閉じタグなので null
    if (ctx.pos + 1 < ctx.source.size() && ctx.source[ctx.pos + 1] == '/')
        return nullptr;

    ++ctx.pos; // skip '<'

    auto node = std::make_unique<XMLNode>();
    node->tag = ParseTagName(ctx);

    if (node->tag.empty())
    {
        GX_LOG_ERROR("XMLDocument: Empty tag name at position %zu", ctx.pos);
        return nullptr;
    }

    // 属性をパース
    ParseAttributes(ctx, node->attributes);

    SkipWhitespace(ctx);

    // 自己閉じタグ />
    if (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] == '/')
    {
        ++ctx.pos; // skip '/'
        if (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] == '>')
            ++ctx.pos; // skip '>'
        return node;
    }

    // '>' を期待
    if (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] == '>')
        ++ctx.pos; // skip '>'
    else
    {
        GX_LOG_ERROR("XMLDocument: Expected '>' for tag '%s' at position %zu",
                     node->tag.c_str(), ctx.pos);
        return nullptr;
    }

    // タグ内容: 子要素 or テキスト
    SkipWhitespace(ctx);
    while (SkipComment(ctx))
        SkipWhitespace(ctx);

    if (ctx.pos < ctx.source.size())
    {
        // 閉じタグチェック
        if (ctx.source[ctx.pos] == '<' && ctx.pos + 1 < ctx.source.size() &&
            ctx.source[ctx.pos + 1] == '/')
        {
            // 空要素 — 閉じタグへ
        }
        else if (ctx.source[ctx.pos] == '<')
        {
            // 子要素モード
            while (ctx.pos < ctx.source.size())
            {
                SkipWhitespace(ctx);
                while (SkipComment(ctx))
                    SkipWhitespace(ctx);

                // 閉じタグなら終了
                if (ctx.pos + 1 < ctx.source.size() &&
                    ctx.source[ctx.pos] == '<' && ctx.source[ctx.pos + 1] == '/')
                    break;

                if (ctx.source[ctx.pos] != '<')
                    break;

                auto child = ParseNode(ctx);
                if (!child) break;
                node->children.push_back(std::move(child));
            }
        }
        else
        {
            // テキストコンテントモード
            node->text = DecodeEntities(ParseTextContent(ctx));
        }
    }

    // 閉じタグ </tagName> をスキップ
    SkipWhitespace(ctx);
    if (ctx.pos + 1 < ctx.source.size() &&
        ctx.source[ctx.pos] == '<' && ctx.source[ctx.pos + 1] == '/')
    {
        ctx.pos += 2; // skip '</'
        std::string closeTag = ParseTagName(ctx);
        if (closeTag != node->tag)
        {
            GX_LOG_ERROR("XMLDocument: Mismatched tags: expected </%s>, got </%s> at position %zu",
                         node->tag.c_str(), closeTag.c_str(), ctx.pos);
        }
        SkipWhitespace(ctx);
        if (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] == '>')
            ++ctx.pos;
    }

    return node;
}

// ============================================================================
// ParseTagName（タグ名解析）
// ============================================================================

std::string XMLDocument::ParseTagName(ParseContext& ctx)
{
    size_t start = ctx.pos;
    while (ctx.pos < ctx.source.size() &&
           (std::isalnum(static_cast<unsigned char>(ctx.source[ctx.pos])) ||
            ctx.source[ctx.pos] == '_' || ctx.source[ctx.pos] == '-'))
    {
        ++ctx.pos;
    }
    return ctx.source.substr(start, ctx.pos - start);
}

// ============================================================================
// ParseAttributes（属性解析）
// ============================================================================

void XMLDocument::ParseAttributes(ParseContext& ctx,
                                   std::unordered_map<std::string, std::string>& attrs)
{
    while (ctx.pos < ctx.source.size())
    {
        SkipWhitespace(ctx);

        char c = ctx.source[ctx.pos];
        if (c == '>' || c == '/')
            break;

        // 属性名
        std::string name = ParseTagName(ctx);
        if (name.empty()) break;

        SkipWhitespace(ctx);

        // '=' を期待
        if (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] == '=')
        {
            ++ctx.pos;
            SkipWhitespace(ctx);
            std::string value = ParseAttributeValue(ctx);
            attrs[name] = DecodeEntities(value);
        }
        else
        {
            // 値なし属性 (e.g. "disabled")
            attrs[name] = "true";
        }
    }
}

// ============================================================================
// ParseAttributeValue（属性値解析）
// ============================================================================

std::string XMLDocument::ParseAttributeValue(ParseContext& ctx)
{
    if (ctx.pos >= ctx.source.size())
        return "";

    char quote = ctx.source[ctx.pos];
    if (quote == '"' || quote == '\'')
    {
        ++ctx.pos;
        size_t start = ctx.pos;
        while (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] != quote)
            ++ctx.pos;
        std::string value = ctx.source.substr(start, ctx.pos - start);
        if (ctx.pos < ctx.source.size())
            ++ctx.pos; // skip closing quote
        return value;
    }

    // クォートなし（属性値としてスペースまで読む）
    size_t start = ctx.pos;
    while (ctx.pos < ctx.source.size() &&
           !std::isspace(static_cast<unsigned char>(ctx.source[ctx.pos])) &&
           ctx.source[ctx.pos] != '>' && ctx.source[ctx.pos] != '/')
    {
        ++ctx.pos;
    }
    return ctx.source.substr(start, ctx.pos - start);
}

// ============================================================================
// ParseTextContent（テキスト内容解析）
// ============================================================================

std::string XMLDocument::ParseTextContent(ParseContext& ctx)
{
    size_t start = ctx.pos;
    while (ctx.pos < ctx.source.size() && ctx.source[ctx.pos] != '<')
        ++ctx.pos;

    std::string text = ctx.source.substr(start, ctx.pos - start);

    // 前後の空白をトリム
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

// ============================================================================
// SkipWhitespace（空白スキップ）
// ============================================================================

void XMLDocument::SkipWhitespace(ParseContext& ctx)
{
    while (ctx.pos < ctx.source.size() &&
           std::isspace(static_cast<unsigned char>(ctx.source[ctx.pos])))
    {
        ++ctx.pos;
    }
}

// ============================================================================
// SkipComment  <!-- ... -->（コメントスキップ）
// ============================================================================

bool XMLDocument::SkipComment(ParseContext& ctx)
{
    if (ctx.pos + 3 < ctx.source.size() &&
        ctx.source[ctx.pos] == '<' && ctx.source[ctx.pos + 1] == '!' &&
        ctx.source[ctx.pos + 2] == '-' && ctx.source[ctx.pos + 3] == '-')
    {
        ctx.pos += 4;
        while (ctx.pos + 2 < ctx.source.size())
        {
            if (ctx.source[ctx.pos] == '-' && ctx.source[ctx.pos + 1] == '-' &&
                ctx.source[ctx.pos + 2] == '>')
            {
                ctx.pos += 3;
                return true;
            }
            ++ctx.pos;
        }
        ctx.pos = ctx.source.size();
        return true;
    }
    return false;
}

// ============================================================================
// SkipXMLDeclaration  <?xml ... ?>（XML宣言スキップ）
// ============================================================================

void XMLDocument::SkipXMLDeclaration(ParseContext& ctx)
{
    if (ctx.pos + 1 < ctx.source.size() &&
        ctx.source[ctx.pos] == '<' && ctx.source[ctx.pos + 1] == '?')
    {
        ctx.pos += 2;
        while (ctx.pos + 1 < ctx.source.size())
        {
            if (ctx.source[ctx.pos] == '?' && ctx.source[ctx.pos + 1] == '>')
            {
                ctx.pos += 2;
                return;
            }
            ++ctx.pos;
        }
    }
}

// ============================================================================
// DecodeEntities（エンティティ復号）
// ============================================================================

std::string XMLDocument::DecodeEntities(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    size_t i = 0;
    while (i < text.size())
    {
        if (text[i] == '&')
        {
            // &amp; &lt; &gt; &quot; &apos;
            if (text.compare(i, 5, "&amp;") == 0)       { result += '&';  i += 5; }
            else if (text.compare(i, 4, "&lt;") == 0)    { result += '<';  i += 4; }
            else if (text.compare(i, 4, "&gt;") == 0)    { result += '>';  i += 4; }
            else if (text.compare(i, 6, "&quot;") == 0)  { result += '"';  i += 6; }
            else if (text.compare(i, 6, "&apos;") == 0)  { result += '\''; i += 6; }
            else { result += text[i]; ++i; } // 未知エンティティはそのまま
        }
        else
        {
            result += text[i];
            ++i;
        }
    }

    return result;
}

}} // namespace GX::GUI
