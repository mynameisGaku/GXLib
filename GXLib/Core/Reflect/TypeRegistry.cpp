/// @file TypeRegistry.cpp
/// @brief TypeRegistry の実装
#include "pch_common.h"
#include "Core/Reflect/TypeRegistry.h"
#include "Core/Logger.h"

namespace gx
{

// ---------------------------------------------------------------------------
// シングルトン
// ---------------------------------------------------------------------------
TypeRegistry& TypeRegistry::Instance()
{
    static TypeRegistry s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// 登録
// ---------------------------------------------------------------------------
void TypeRegistry::RegisterType(std::unique_ptr<TypeInfo> type)
{
    if (!type) return;

    const gx::String name = type->GetName();
    uint32_t hash = type->GetTypeHash();

    // 重複登録のチェック
    if (m_types.find(name) != m_types.end())
    {
        GX_LOG_WARN("TypeRegistry: type '%s' is already registered, overwriting", name.c_str());
        // 旧ハッシュマッピングを削除
        auto it = m_hashMap.find(m_types[name]->GetTypeHash());
        if (it != m_hashMap.end())
            m_hashMap.erase(it);
    }

    TypeInfo* raw = type.get();
    m_types[name] = std::move(type);
    m_hashMap[hash] = raw;

    GX_LOG_INFO("TypeRegistry: registered type '%s' (hash=0x%08X, size=%u, props=%u)",
                name.c_str(), hash, raw->GetTypeSize(), raw->GetPropertyCount());
}

// ---------------------------------------------------------------------------
// 検索
// ---------------------------------------------------------------------------
const TypeInfo* TypeRegistry::FindType(const gx::String& name) const
{
    auto it = m_types.find(name);
    return (it != m_types.end()) ? it->second.get() : nullptr;
}

const TypeInfo* TypeRegistry::FindTypeByHash(uint32_t hash) const
{
    auto it = m_hashMap.find(hash);
    return (it != m_hashMap.end()) ? it->second : nullptr;
}

gx::Vector<const TypeInfo*> TypeRegistry::GetAllTypes() const
{
    gx::Vector<const TypeInfo*> result;
    result.reserve(m_types.size());
    for (const auto& [name, info] : m_types)
    {
        result.push_back(info.get());
    }
    return result;
}

// ---------------------------------------------------------------------------
// クリア
// ---------------------------------------------------------------------------
void TypeRegistry::Clear()
{
    m_hashMap.clear();
    m_types.clear();
}

} // namespace gx
