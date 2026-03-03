/// @file Allocator.cpp
/// @brief アロケータ実装

#include "pch_common.h"
#include "Container/Allocator.h"
#include <cstdlib>

namespace gx::container {

static Allocator s_defaultAllocator("gx::default");
static Allocator* s_pDefaultAllocator = &s_defaultAllocator;

Allocator::Allocator(const char* name)
    : m_name(name)
{
}

void* Allocator::allocate(size_t n, int /*flags*/)
{
    return _aligned_malloc(n, 16);
}

void* Allocator::allocate(size_t n, size_t alignment, size_t /*offset*/, int /*flags*/)
{
    return _aligned_malloc(n, alignment);
}

void Allocator::deallocate(void* p, size_t /*n*/)
{
    _aligned_free(p);
}

Allocator* GetDefaultAllocator()
{
    return s_pDefaultAllocator;
}

void SetDefaultAllocator(Allocator* allocator)
{
    s_pDefaultAllocator = allocator ? allocator : &s_defaultAllocator;
}

// FixedBufferAllocator
FixedBufferAllocator::FixedBufferAllocator(void* buffer, size_t bufferSize)
    : m_buffer(static_cast<char*>(buffer)), m_bufferSize(bufferSize), m_currentOffset(0)
{
}

void FixedBufferAllocator::Init(void* buffer, size_t bufferSize)
{
    m_buffer = static_cast<char*>(buffer);
    m_bufferSize = bufferSize;
    m_currentOffset = 0;
}

void* FixedBufferAllocator::allocate(size_t n, int /*flags*/)
{
    return allocate(n, 16, 0, 0);
}

void* FixedBufferAllocator::allocate(size_t n, size_t alignment, size_t /*offset*/, int /*flags*/)
{
    size_t aligned = (m_currentOffset + alignment - 1) & ~(alignment - 1);
    if (aligned + n > m_bufferSize)
    {
        assert(false && "FixedBufferAllocator overflow");
        return nullptr;
    }
    void* p = m_buffer + aligned;
    m_currentOffset = aligned + n;
    return p;
}

void FixedBufferAllocator::deallocate(void* /*p*/, size_t /*n*/)
{
    // Fixed buffer allocator does not deallocate individual blocks
}

bool FixedBufferAllocator::OwnsMemory(const void* p) const
{
    auto cp = static_cast<const char*>(p);
    return cp >= m_buffer && cp < m_buffer + m_bufferSize;
}

} // namespace gx::container
