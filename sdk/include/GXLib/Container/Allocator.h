#pragma once
/// @file Allocator.h
/// @brief ゲームエンジン向け非テンプレートアロケータ

#include "pch_common.h"

namespace gx::container {

/// @brief アロケータフラグ
enum class AllocFlags : uint32_t
{
    None = 0,
    TempMemory = 1,  // 一時メモリ
};

/// @brief 非テンプレートアロケータ基底
class Allocator
{
public:
    explicit Allocator(const char* name = "gx::container::Allocator");
    Allocator(const Allocator&) = default;
    Allocator& operator=(const Allocator&) = default;
    ~Allocator() = default;

    void* allocate(size_t n, int flags = 0);
    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
    void  deallocate(void* p, size_t n);

    const char* GetName() const { return m_name; }
    void SetName(const char* name) { m_name = name; }

private:
    const char* m_name;
};

__forceinline bool operator==(const Allocator& a, const Allocator& b) { return &a == &b; }
__forceinline bool operator!=(const Allocator& a, const Allocator& b) { return &a != &b; }

Allocator* GetDefaultAllocator();
void SetDefaultAllocator(Allocator* allocator);

/// @brief 固定バッファアロケータ (FixedContainer用)
class FixedBufferAllocator
{
public:
    FixedBufferAllocator() = default;
    FixedBufferAllocator(void* buffer, size_t bufferSize);

    void* allocate(size_t n, int flags = 0);
    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
    void  deallocate(void* p, size_t n);

    void Init(void* buffer, size_t bufferSize);
    bool OwnsMemory(const void* p) const;

    const char* GetName() const { return "FixedBufferAllocator"; }
    void SetName(const char*) {}

private:
    char*  m_buffer = nullptr;
    size_t m_bufferSize = 0;
    size_t m_currentOffset = 0;
};

} // namespace gx::container
