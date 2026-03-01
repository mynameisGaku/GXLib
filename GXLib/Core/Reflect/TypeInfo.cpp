/// @file TypeInfo.cpp
/// @brief TypeInfo の実装
#include "pch_common.h"
#include "Core/Reflect/TypeInfo.h"
#include "Core/Logger.h"

namespace gx
{

// ---------------------------------------------------------------------------
// コンストラクタ
// ---------------------------------------------------------------------------
TypeInfo::TypeInfo(const std::string& name, uint32_t typeSize)
    : m_name(name)
    , m_typeSize(typeSize)
    , m_typeHash(HashString(name))
{
}

// ---------------------------------------------------------------------------
// プロパティ登録
// ---------------------------------------------------------------------------
void TypeInfo::AddProperty(const PropertyMeta& prop)
{
    m_properties.push_back(prop);
}

const PropertyMeta* TypeInfo::FindProperty(const std::string& name) const
{
    for (const auto& p : m_properties)
    {
        if (p.name == name)
            return &p;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// ゲッター — (char*)obj + offset から生バイトを読み取る
// ---------------------------------------------------------------------------
bool TypeInfo::GetBool(const void* obj, const std::string& propName) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Bool) return false;
    return *reinterpret_cast<const bool*>(static_cast<const char*>(obj) + p->offset);
}

int TypeInfo::GetInt(const void* obj, const std::string& propName) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Int) return 0;
    return *reinterpret_cast<const int*>(static_cast<const char*>(obj) + p->offset);
}

float TypeInfo::GetFloat(const void* obj, const std::string& propName) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Float) return 0.0f;
    return *reinterpret_cast<const float*>(static_cast<const char*>(obj) + p->offset);
}

std::string TypeInfo::GetString(const void* obj, const std::string& propName) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::String) return {};

    // std::stringは非トリビアルコピー型のためゲッター/セッターを使用
    if (p->getter)
    {
        std::string result;
        p->getter(obj, &result);
        return result;
    }
    // フォールバック: 直接メモリ読み取り（レイアウトが一致する場合のみ安全）
    return *reinterpret_cast<const std::string*>(static_cast<const char*>(obj) + p->offset);
}

// ---------------------------------------------------------------------------
// セッター — (char*)obj + offset に生バイトを書き込む
// ---------------------------------------------------------------------------
void TypeInfo::SetBool(void* obj, const std::string& propName, bool value) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Bool || p->readOnly) return;
    *reinterpret_cast<bool*>(static_cast<char*>(obj) + p->offset) = value;
}

void TypeInfo::SetInt(void* obj, const std::string& propName, int value) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Int || p->readOnly) return;
    if (p->hasRange)
        value = (std::max)(static_cast<int>(p->minValue), (std::min)(static_cast<int>(p->maxValue), value));
    *reinterpret_cast<int*>(static_cast<char*>(obj) + p->offset) = value;
}

void TypeInfo::SetFloat(void* obj, const std::string& propName, float value) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::Float || p->readOnly) return;
    if (p->hasRange)
        value = (std::max)(p->minValue, (std::min)(p->maxValue, value));
    *reinterpret_cast<float*>(static_cast<char*>(obj) + p->offset) = value;
}

void TypeInfo::SetString(void* obj, const std::string& propName, const std::string& value) const
{
    const PropertyMeta* p = FindProperty(propName);
    if (!p || p->type != PropertyType::String || p->readOnly) return;

    if (p->setter)
    {
        p->setter(obj, &value);
        return;
    }
    // フォールバック: 直接メモリ書き込み
    *reinterpret_cast<std::string*>(static_cast<char*>(obj) + p->offset) = value;
}

// ---------------------------------------------------------------------------
// FNV-1aハッシュ
// ---------------------------------------------------------------------------
uint32_t TypeInfo::HashString(const std::string& s)
{
    uint32_t hash = 2166136261u;
    for (char c : s)
    {
        hash ^= static_cast<uint32_t>(static_cast<uint8_t>(c));
        hash *= 16777619u;
    }
    return hash;
}

} // namespace gx
