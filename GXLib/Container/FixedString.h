#pragma once
/// @file FixedString.h
/// @brief 固定容量文字列 — 内部バッファ使用、ヒープ退避オプション付き

#include "pch_common.h"
#include "Container/Allocator.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量文字列
/// @tparam N 内部バッファ容量 (null 終端を含まない最大文字数)
/// @tparam AllowOverflow true なら満杯時にヒープへ退避
template <size_t N, bool AllowOverflow = false>
class FixedString
{
public:
    using value_type      = char;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = char&;
    using const_reference = const char&;
    using pointer         = char*;
    using const_pointer   = const char*;
    using iterator        = char*;
    using const_iterator  = const char*;

    static constexpr size_type kFixedCapacity = N;
    static constexpr size_type npos = static_cast<size_type>(-1);

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedString() noexcept
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        m_buffer[0] = '\0';
    }

    FixedString(const char* str)
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        if (str)
        {
            size_type len = std::strlen(str);
            EnsureCapacity(len);
            std::memcpy(m_data, str, len);
            m_size = len;
            m_data[m_size] = '\0';
        }
        else
        {
            m_buffer[0] = '\0';
        }
    }

    FixedString(const char* str, size_type count)
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        m_buffer[0] = '\0';
        EnsureCapacity(count);
        std::memcpy(m_data, str, count);
        m_size = count;
        m_data[m_size] = '\0';
    }

    FixedString(size_type count, char ch)
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        m_buffer[0] = '\0';
        EnsureCapacity(count);
        std::memset(m_data, ch, count);
        m_size = count;
        m_data[m_size] = '\0';
    }

    FixedString(std::string_view sv)
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        m_buffer[0] = '\0';
        EnsureCapacity(sv.size());
        std::memcpy(m_data, sv.data(), sv.size());
        m_size = sv.size();
        m_data[m_size] = '\0';
    }

    FixedString(const FixedString& other)
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        m_buffer[0] = '\0';
        EnsureCapacity(other.m_size);
        std::memcpy(m_data, other.m_data, other.m_size);
        m_size = other.m_size;
        m_data[m_size] = '\0';
    }

    FixedString(FixedString&& other) noexcept
        : m_data(m_buffer), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        if (other.m_usingHeap)
        {
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_usingHeap = true;
            other.m_data = other.m_buffer;
            other.m_size = 0;
            other.m_capacity = N;
            other.m_usingHeap = false;
            other.m_buffer[0] = '\0';
        }
        else
        {
            std::memcpy(m_buffer, other.m_buffer, other.m_size + 1);
            m_size = other.m_size;
            other.m_size = 0;
            other.m_buffer[0] = '\0';
        }
    }

    ~FixedString() { FreeHeap(); }

    FixedString& operator=(const FixedString& other)
    {
        if (this != &other)
        {
            EnsureCapacity(other.m_size);
            std::memcpy(m_data, other.m_data, other.m_size);
            m_size = other.m_size;
            m_data[m_size] = '\0';
        }
        return *this;
    }

    FixedString& operator=(FixedString&& other) noexcept
    {
        if (this != &other)
        {
            FreeHeap();
            if (other.m_usingHeap)
            {
                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                m_usingHeap = true;
                other.m_data = other.m_buffer;
                other.m_size = 0;
                other.m_capacity = N;
                other.m_usingHeap = false;
                other.m_buffer[0] = '\0';
            }
            else
            {
                m_data = m_buffer;
                m_capacity = N;
                m_usingHeap = false;
                std::memcpy(m_buffer, other.m_buffer, other.m_size + 1);
                m_size = other.m_size;
                other.m_size = 0;
                other.m_buffer[0] = '\0';
            }
        }
        return *this;
    }

    FixedString& operator=(const char* str)
    {
        size_type len = str ? std::strlen(str) : 0;
        EnsureCapacity(len);
        if (len > 0) std::memcpy(m_data, str, len);
        m_size = len;
        m_data[m_size] = '\0';
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    __forceinline reference operator[](size_type i) noexcept { assert(i < m_size); return m_data[i]; }
    __forceinline const_reference operator[](size_type i) const noexcept { assert(i < m_size); return m_data[i]; }
    __forceinline reference       Front()       noexcept { assert(m_size > 0); return m_data[0]; }
    __forceinline const_reference Front() const noexcept { assert(m_size > 0); return m_data[0]; }
    __forceinline reference       Back()        noexcept { assert(m_size > 0); return m_data[m_size - 1]; }
    __forceinline const_reference Back()  const noexcept { assert(m_size > 0); return m_data[m_size - 1]; }
    __forceinline pointer         Data()        noexcept { return m_data; }
    __forceinline const_pointer   Data()  const noexcept { return m_data; }
    __forceinline const_pointer   CStr()  const noexcept { return m_data; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return m_data; }
    __forceinline const_iterator begin()  const noexcept { return m_data; }
    __forceinline const_iterator cbegin() const noexcept { return m_data; }
    __forceinline iterator       end()          noexcept { return m_data + m_size; }
    __forceinline const_iterator end()    const noexcept { return m_data + m_size; }
    __forceinline const_iterator cend()   const noexcept { return m_data + m_size; }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline size_type Length() const noexcept { return m_size; }
    __forceinline size_type Capacity() const noexcept { return m_capacity; }
    __forceinline bool      Empty() const noexcept { return m_size == 0; }
    __forceinline bool      IsUsingHeap() const noexcept { return m_usingHeap; }

    // ========================================================================
    // Conversion
    // ========================================================================

    operator std::string_view() const noexcept { return std::string_view(m_data, m_size); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    void Clear() noexcept
    {
        m_size = 0;
        m_data[0] = '\0';
    }

    void Resize(size_type newSize, char ch = '\0')
    {
        if (newSize > m_size)
        {
            EnsureCapacity(newSize);
            std::memset(m_data + m_size, ch, newSize - m_size);
        }
        m_size = newSize;
        m_data[m_size] = '\0';
    }

    void PushBack(char ch)
    {
        EnsureCapacity(m_size + 1);
        m_data[m_size] = ch;
        ++m_size;
        m_data[m_size] = '\0';
    }

    void PopBack() noexcept
    {
        assert(m_size > 0);
        --m_size;
        m_data[m_size] = '\0';
    }

    FixedString& Append(const char* str, size_type len)
    {
        EnsureCapacity(m_size + len);
        std::memcpy(m_data + m_size, str, len);
        m_size += len;
        m_data[m_size] = '\0';
        return *this;
    }

    FixedString& Append(const char* str) { return Append(str, std::strlen(str)); }
    FixedString& Append(std::string_view sv) { return Append(sv.data(), sv.size()); }

    FixedString& operator+=(const char* str) { return Append(str); }
    FixedString& operator+=(char ch) { PushBack(ch); return *this; }
    FixedString& operator+=(std::string_view sv) { return Append(sv); }

    // ========================================================================
    // Search
    // ========================================================================

    size_type Find(char ch, size_type pos = 0) const noexcept
    {
        for (size_type i = pos; i < m_size; ++i)
            if (m_data[i] == ch) return i;
        return npos;
    }

    size_type Find(const char* str, size_type pos = 0) const noexcept
    {
        if (!str) return npos;
        size_type len = std::strlen(str);
        if (len == 0) return pos <= m_size ? pos : npos;
        if (len > m_size) return npos;
        for (size_type i = pos; i + len <= m_size; ++i)
            if (std::memcmp(m_data + i, str, len) == 0) return i;
        return npos;
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const FixedString& a, const FixedString& b) noexcept
    {
        return a.m_size == b.m_size && std::memcmp(a.m_data, b.m_data, a.m_size) == 0;
    }

    friend bool operator!=(const FixedString& a, const FixedString& b) noexcept { return !(a == b); }

    friend bool operator==(const FixedString& a, const char* b) noexcept
    {
        size_type blen = b ? std::strlen(b) : 0;
        return a.m_size == blen && std::memcmp(a.m_data, b, blen) == 0;
    }

    friend bool operator!=(const FixedString& a, const char* b) noexcept { return !(a == b); }

    friend bool operator<(const FixedString& a, const FixedString& b) noexcept
    {
        size_type minLen = a.m_size < b.m_size ? a.m_size : b.m_size;
        int cmp = std::memcmp(a.m_data, b.m_data, minLen);
        return cmp < 0 || (cmp == 0 && a.m_size < b.m_size);
    }

    // ========================================================================
    // Validation
    // ========================================================================

    bool Validate() const noexcept { return m_size <= m_capacity && m_data[m_size] == '\0'; }

private:
    void EnsureCapacity(size_type required)
    {
        if (required <= m_capacity)
            return;
        if constexpr (!AllowOverflow)
        {
            assert(false && "FixedString overflow (AllowOverflow=false)");
        }
        else
        {
            size_type newCap = m_capacity + m_capacity / 2;
            if (newCap < required) newCap = required;
            GrowTo(newCap);
        }
    }

    void GrowTo(size_type newCap)
    {
        Allocator alloc;
        char* newData = static_cast<char*>(alloc.allocate(newCap + 1, 1, 0));
        std::memcpy(newData, m_data, m_size + 1);
        FreeHeap();
        m_data = newData;
        m_capacity = newCap;
        m_usingHeap = true;
    }

    void FreeHeap()
    {
        if (m_usingHeap)
        {
            Allocator alloc;
            alloc.deallocate(m_data, m_capacity + 1);
            m_data = m_buffer;
            m_capacity = N;
            m_usingHeap = false;
        }
    }

    char      m_buffer[N + 1];
    char*     m_data;
    size_type m_size;
    size_type m_capacity;
    bool      m_usingHeap;
};

} // namespace gx::container
