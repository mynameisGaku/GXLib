/// @file JsonSerializer.cpp
/// @brief JsonSerializer の実装
#include "pch_common.h"
#include "Core/Reflect/JsonSerializer.h"
#include "Core/Logger.h"

namespace gx
{

// ---------------------------------------------------------------------------
// 内部ヘルパー
// ---------------------------------------------------------------------------
namespace
{

/// JSON出力用に文字列をエスケープする（クォートとバックスラッシュを処理）
gx::String EscapeJsonString(const gx::String& s)
{
    gx::String out;
    out.reserve(s.size() + 2);
    for (char c : s)
    {
        switch (c)
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

/// JSON文字列値のアンエスケープ
gx::String UnescapeJsonString(const gx::String& s)
{
    gx::String out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            ++i;
            switch (s[i])
            {
            case '"':  out += '"';  break;
            case '\\': out += '\\'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            default:   out += s[i]; break;
            }
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

/// 先頭/末尾の空白を除去
gx::String Trim(const gx::String& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == gx::String::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

/// オフセット位置のオブジェクト生メモリからfloatを読み取る
float ReadFloat(const void* obj, uint32_t offset)
{
    return *reinterpret_cast<const float*>(static_cast<const char*>(obj) + offset);
}

/// オフセット位置のオブジェクト生メモリにfloatを書き込む
void WriteFloat(void* obj, uint32_t offset, float value)
{
    *reinterpret_cast<float*>(static_cast<char*>(obj) + offset) = value;
}

/// floatを末尾ゼロなしでフォーマットする（最低1桁の小数は保持）
gx::String FloatToString(float v)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(v));
    return buf;
}

/// JSONオブジェクト文字列から指定キーの値文字列を検索する
/// 見つからない場合は空文字列を返す
gx::String FindJsonValue(const gx::String& json, const gx::String& key)
{
    // "key":を検索
    gx::String searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == gx::String::npos) return {};

    // コロンをスキップ
    pos = json.find(':', pos + searchKey.size());
    if (pos == gx::String::npos) return {};
    ++pos;

    // 空白をスキップ
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;

    if (pos >= json.size()) return {};

    // 値の型を判定
    if (json[pos] == '"')
    {
        // 文字列値 — 閉じクォートを検索（エスケープを考慮）
        size_t start = pos + 1;
        size_t end = start;
        while (end < json.size())
        {
            if (json[end] == '\\') { end += 2; continue; }
            if (json[end] == '"') break;
            ++end;
        }
        return json.substr(start, end - start);
    }
    else if (json[pos] == '{' || json[pos] == '[')
    {
        // オブジェクトまたは配列 — 対応する閉じ括弧を検索
        char open = json[pos];
        char close = (open == '{') ? '}' : ']';
        int depth = 1;
        size_t start = pos;
        ++pos;
        while (pos < json.size() && depth > 0)
        {
            if (json[pos] == open) ++depth;
            else if (json[pos] == close) --depth;
            ++pos;
        }
        return json.substr(start, pos - start);
    }
    else
    {
        // プリミティブ（数値、bool、null）
        size_t start = pos;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']'
               && json[pos] != '\n' && json[pos] != '\r')
            ++pos;
        return Trim(json.substr(start, pos - start));
    }
}

/// JSONサブオブジェクトからキーでfloatを抽出する（例: {"x": 1.0} => 1.0）
float ExtractFloatKey(const gx::String& json, const gx::String& key)
{
    gx::String val = FindJsonValue(json, key);
    if (val.empty()) return 0.0f;
    return static_cast<float>(atof(val.c_str()));
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// SerializeProperty
// ---------------------------------------------------------------------------
gx::String JsonSerializer::SerializeProperty(const PropertyMeta& prop, const void* obj)
{
    const char* base = static_cast<const char*>(obj);

    switch (prop.type)
    {
    case PropertyType::Bool:
    {
        bool v = *reinterpret_cast<const bool*>(base + prop.offset);
        return gx::String("\"") + prop.name + "\": " + (v ? "true" : "false");
    }
    case PropertyType::Int:
    {
        int v = *reinterpret_cast<const int*>(base + prop.offset);
        return "\"" + prop.name + "\": " + std::to_string(v);
    }
    case PropertyType::Float:
    {
        float v = ReadFloat(obj, prop.offset);
        return "\"" + prop.name + "\": " + FloatToString(v);
    }
    case PropertyType::String:
    {
        gx::String v;
        if (prop.getter)
            prop.getter(obj, &v);
        else
            v = *reinterpret_cast<const gx::String*>(base + prop.offset);
        return "\"" + prop.name + "\": \"" + EscapeJsonString(v) + "\"";
    }
    case PropertyType::Vec3:
    {
        float x = ReadFloat(obj, prop.offset);
        float y = ReadFloat(obj, prop.offset + 4);
        float z = ReadFloat(obj, prop.offset + 8);
        return "\"" + prop.name + "\": {\"x\": " + FloatToString(x)
             + ", \"y\": " + FloatToString(y) + ", \"z\": " + FloatToString(z) + "}";
    }
    case PropertyType::Vec4:
    {
        float x = ReadFloat(obj, prop.offset);
        float y = ReadFloat(obj, prop.offset + 4);
        float z = ReadFloat(obj, prop.offset + 8);
        float w = ReadFloat(obj, prop.offset + 12);
        return "\"" + prop.name + "\": {\"x\": " + FloatToString(x)
             + ", \"y\": " + FloatToString(y) + ", \"z\": " + FloatToString(z)
             + ", \"w\": " + FloatToString(w) + "}";
    }
    case PropertyType::Quaternion:
    {
        float x = ReadFloat(obj, prop.offset);
        float y = ReadFloat(obj, prop.offset + 4);
        float z = ReadFloat(obj, prop.offset + 8);
        float w = ReadFloat(obj, prop.offset + 12);
        return "\"" + prop.name + "\": {\"x\": " + FloatToString(x)
             + ", \"y\": " + FloatToString(y) + ", \"z\": " + FloatToString(z)
             + ", \"w\": " + FloatToString(w) + "}";
    }
    case PropertyType::Color:
    {
        float r = ReadFloat(obj, prop.offset);
        float g = ReadFloat(obj, prop.offset + 4);
        float b = ReadFloat(obj, prop.offset + 8);
        float a = ReadFloat(obj, prop.offset + 12);
        return "\"" + prop.name + "\": {\"r\": " + FloatToString(r)
             + ", \"g\": " + FloatToString(g) + ", \"b\": " + FloatToString(b)
             + ", \"a\": " + FloatToString(a) + "}";
    }
    case PropertyType::Enum:
    {
        int v = *reinterpret_cast<const int*>(base + prop.offset);
        return "\"" + prop.name + "\": " + std::to_string(v);
    }
    case PropertyType::Custom:
    default:
        return "\"" + prop.name + "\": null";
    }
}

// ---------------------------------------------------------------------------
// Serialize
// ---------------------------------------------------------------------------
gx::String JsonSerializer::Serialize(const TypeInfo& type, const void* obj)
{
    gx::String result = "{\n  \"_type\": \"" + type.GetName() + "\"";

    for (const auto& prop : type.GetProperties())
    {
        result += ",\n  ";
        result += SerializeProperty(prop, obj);
    }

    result += "\n}";
    return result;
}

// ---------------------------------------------------------------------------
// DeserializeProperty
// ---------------------------------------------------------------------------
bool JsonSerializer::DeserializeProperty(const PropertyMeta& prop, void* obj, const gx::String& value)
{
    char* base = static_cast<char*>(obj);
    if (prop.readOnly) return false;

    switch (prop.type)
    {
    case PropertyType::Bool:
    {
        gx::String trimmed = Trim(value);
        bool v = (trimmed == "true" || trimmed == "1");
        *reinterpret_cast<bool*>(base + prop.offset) = v;
        return true;
    }
    case PropertyType::Int:
    {
        int v = atoi(value.c_str());
        if (prop.hasRange)
            v = (std::max)(static_cast<int>(prop.minValue), (std::min)(static_cast<int>(prop.maxValue), v));
        *reinterpret_cast<int*>(base + prop.offset) = v;
        return true;
    }
    case PropertyType::Float:
    {
        float v = static_cast<float>(atof(value.c_str()));
        if (prop.hasRange)
            v = (std::max)(prop.minValue, (std::min)(prop.maxValue, v));
        WriteFloat(obj, prop.offset, v);
        return true;
    }
    case PropertyType::String:
    {
        gx::String unescaped = UnescapeJsonString(value);
        if (prop.setter)
            prop.setter(obj, &unescaped);
        else
            *reinterpret_cast<gx::String*>(base + prop.offset) = unescaped;
        return true;
    }
    case PropertyType::Vec3:
    {
        WriteFloat(obj, prop.offset,     ExtractFloatKey(value, "x"));
        WriteFloat(obj, prop.offset + 4, ExtractFloatKey(value, "y"));
        WriteFloat(obj, prop.offset + 8, ExtractFloatKey(value, "z"));
        return true;
    }
    case PropertyType::Vec4:
    case PropertyType::Quaternion:
    {
        WriteFloat(obj, prop.offset,      ExtractFloatKey(value, "x"));
        WriteFloat(obj, prop.offset + 4,  ExtractFloatKey(value, "y"));
        WriteFloat(obj, prop.offset + 8,  ExtractFloatKey(value, "z"));
        WriteFloat(obj, prop.offset + 12, ExtractFloatKey(value, "w"));
        return true;
    }
    case PropertyType::Color:
    {
        WriteFloat(obj, prop.offset,      ExtractFloatKey(value, "r"));
        WriteFloat(obj, prop.offset + 4,  ExtractFloatKey(value, "g"));
        WriteFloat(obj, prop.offset + 8,  ExtractFloatKey(value, "b"));
        WriteFloat(obj, prop.offset + 12, ExtractFloatKey(value, "a"));
        return true;
    }
    case PropertyType::Enum:
    {
        int v = atoi(value.c_str());
        *reinterpret_cast<int*>(base + prop.offset) = v;
        return true;
    }
    case PropertyType::Custom:
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// Deserialize
// ---------------------------------------------------------------------------
bool JsonSerializer::Deserialize(const TypeInfo& type, void* obj, const gx::String& json)
{
    if (json.empty()) return false;

    bool anySuccess = false;
    for (const auto& prop : type.GetProperties())
    {
        gx::String rawValue = FindJsonValue(json, prop.name);
        if (!rawValue.empty())
        {
            if (DeserializeProperty(prop, obj, rawValue))
                anySuccess = true;
        }
    }
    return anySuccess;
}

} // namespace gx
