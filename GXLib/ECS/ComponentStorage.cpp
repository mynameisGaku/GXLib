/// @file ComponentStorage.cpp
/// @brief ComponentStorage の実装
#include "pch_common.h"
#include "ECS/ComponentStorage.h"

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// 構築 / 破棄
// ---------------------------------------------------------------------------
ComponentStorage::ComponentStorage(ComponentID id, uint32_t componentSize, const std::string& name)
    : m_componentId(id)
    , m_componentSize(componentSize)
    , m_name(name)
{
}

ComponentStorage::~ComponentStorage() = default;

ComponentStorage::ComponentStorage(ComponentStorage&& other) noexcept
    : m_componentId(other.m_componentId)
    , m_componentSize(other.m_componentSize)
    , m_name(std::move(other.m_name))
    , m_data(std::move(other.m_data))
    , m_count(other.m_count)
    , m_capacity(other.m_capacity)
{
    other.m_count = 0;
    other.m_capacity = 0;
}

ComponentStorage& ComponentStorage::operator=(ComponentStorage&& other) noexcept
{
    if (this != &other)
    {
        m_componentId = other.m_componentId;
        m_componentSize = other.m_componentSize;
        m_name = std::move(other.m_name);
        m_data = std::move(other.m_data);
        m_count = other.m_count;
        m_capacity = other.m_capacity;
        other.m_count = 0;
        other.m_capacity = 0;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// 要素管理
// ---------------------------------------------------------------------------
void* ComponentStorage::AddElement()
{
    EnsureCapacity(m_count + 1);
    void* ptr = m_data.data() + static_cast<size_t>(m_count) * m_componentSize;
    // 新しい要素をゼロ初期化
    memset(ptr, 0, m_componentSize);
    ++m_count;
    return ptr;
}

void ComponentStorage::RemoveElement(uint32_t index)
{
    SwapAndPop(index);
}

void* ComponentStorage::GetElement(uint32_t index)
{
    assert(index < m_count);
    return m_data.data() + static_cast<size_t>(index) * m_componentSize;
}

const void* ComponentStorage::GetElement(uint32_t index) const
{
    assert(index < m_count);
    return m_data.data() + static_cast<size_t>(index) * m_componentSize;
}

void ComponentStorage::SwapAndPop(uint32_t index)
{
    assert(index < m_count);
    if (m_count == 0) return;

    uint32_t lastIndex = m_count - 1;
    if (index != lastIndex)
    {
        // 最後の要素を削除された位置にコピー
        void* dst = m_data.data() + static_cast<size_t>(index) * m_componentSize;
        const void* src = m_data.data() + static_cast<size_t>(lastIndex) * m_componentSize;
        memcpy(dst, src, m_componentSize);
    }
    --m_count;
}

void ComponentStorage::Reserve(uint32_t capacity)
{
    EnsureCapacity(capacity);
}

void ComponentStorage::Clear()
{
    m_count = 0;
}

// ---------------------------------------------------------------------------
// 内部
// ---------------------------------------------------------------------------
void ComponentStorage::EnsureCapacity(uint32_t requiredCount)
{
    if (requiredCount <= m_capacity) return;

    // 最低2倍に拡張（最小16）
    uint32_t newCap = (std::max)(requiredCount, (std::max)(m_capacity * 2, 16u));
    m_data.resize(static_cast<size_t>(newCap) * m_componentSize, 0);
    m_capacity = newCap;
}

}} // namespace gx::ecs
