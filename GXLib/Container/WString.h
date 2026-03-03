#pragma once
/// @file WString.h
/// @brief SSO ワイド文字列 (10文字インライン)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief SSO ワイド文字列クラス
///
/// Small String Optimization: 10文字 (wchar_t) 以下はヒープ確保なしでインライン格納。
/// wchar_t = 2バイト(Windows) の場合、SSO バッファ = 22バイト + 2バイト(null) + フラグ。
class WString
{
public:
    // ========================================================================
    // Type aliases
    // ========================================================================
    using value_type      = wchar_t;
    using size_type       = size_t;
    using reference       = wchar_t&;
    using const_reference = const wchar_t&;
    using pointer         = wchar_t*;
    using const_pointer   = const wchar_t*;
    using iterator        = wchar_t*;
    using const_iterator  = const wchar_t*;
    using allocator_type  = Allocator;

    static constexpr size_type npos = static_cast<size_type>(-1);

    /// @brief SSO 格納可能文字数 (wchar_t単位)
    static constexpr size_type k_SSOCapacity = 10;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ (空文字列、SSO)
    WString() noexcept
    {
        SetSSOSize(0);
        m_sso.buffer[0] = L'\0';
        SyncDebugVars();
    }

    /// @brief ワイド文字列から構築
    WString(const wchar_t* str)
    {
        if (str)
        {
            size_type len = std::wcslen(str);
            DoConstruct(str, len);
        }
        else
        {
            SetSSOSize(0);
            m_sso.buffer[0] = L'\0';
        }
        SyncDebugVars();
    }

    /// @brief ワイド文字列 + 長さから構築
    WString(const wchar_t* str, size_type len)
    {
        DoConstruct(str, len);
        SyncDebugVars();
    }

    /// @brief n個のwchar_tで構築
    WString(size_type n, wchar_t c)
    {
        if (n <= k_SSOCapacity)
        {
            for (size_type i = 0; i < n; ++i)
                m_sso.buffer[i] = c;
            m_sso.buffer[n] = L'\0';
            SetSSOSize(n);
        }
        else
        {
            DoAllocateAndInit(n);
            for (size_type i = 0; i < n; ++i)
                m_heap.data[i] = c;
            m_heap.data[n] = L'\0';
            m_heap.size = n;
        }
        SyncDebugVars();
    }

    /// @brief std::wstring から構築
    WString(const std::wstring& str)
    {
        DoConstruct(str.data(), str.size());
        SyncDebugVars();
    }

    /// @brief std::wstring_view から構築
    WString(std::wstring_view sv)
    {
        DoConstruct(sv.data(), sv.size());
        SyncDebugVars();
    }

    /// @brief コピーコンストラクタ
    WString(const WString& other)
    {
        if (other.IsSSO())
        {
            std::memcpy(this, &other, sizeof(SSOLayout));
        }
        else
        {
            size_type len = other.Size();
            DoAllocateAndInit(len);
            std::memcpy(m_heap.data, other.m_heap.data, (len + 1) * sizeof(wchar_t));
            m_heap.size = len;
        }
        SyncDebugVars();
    }

    /// @brief ムーブコンストラクタ
    WString(WString&& other) noexcept
    {
        std::memcpy(this, &other, sizeof(WString));
        other.SetSSOSize(0);
        other.m_sso.buffer[0] = L'\0';
        other.SyncDebugVars();
    }

    /// @brief デストラクタ
    ~WString()
    {
        if (!IsSSO())
            DoFree();
    }

    // ========================================================================
    // Assignment
    // ========================================================================

    /// @brief コピー代入
    WString& operator=(const WString& other)
    {
        if (this != &other)
        {
            WString tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    /// @brief ムーブ代入
    WString& operator=(WString&& other) noexcept
    {
        if (this != &other)
        {
            if (!IsSSO())
                DoFree();
            std::memcpy(this, &other, sizeof(WString));
            other.SetSSOSize(0);
            other.m_sso.buffer[0] = L'\0';
            other.SyncDebugVars();
        }
        return *this;
    }

    /// @brief ワイド文字列代入
    WString& operator=(const wchar_t* str)
    {
        WString tmp(str);
        Swap(tmp);
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 添字アクセス
    __forceinline reference operator[](size_type i) noexcept
    {
        assert(i < Size());
        return DataMut()[i];
    }

    /// @brief 添字アクセス (const)
    __forceinline const_reference operator[](size_type i) const noexcept
    {
        assert(i < Size());
        return Data()[i];
    }

    /// @brief 先頭文字
    __forceinline reference Front() noexcept { assert(!Empty()); return DataMut()[0]; }
    __forceinline const_reference Front() const noexcept { assert(!Empty()); return Data()[0]; }

    /// @brief 末尾文字
    __forceinline reference Back() noexcept { assert(!Empty()); return DataMut()[Size() - 1]; }
    __forceinline const_reference Back() const noexcept { assert(!Empty()); return Data()[Size() - 1]; }

    /// @brief C文字列取得 (null終端)
    __forceinline const wchar_t* c_str() const noexcept { return Data(); }

    /// @brief 生ポインタ取得
    __forceinline const wchar_t* Data() const noexcept
    {
        return IsSSO() ? m_sso.buffer : m_heap.data;
    }

    /// @brief 生ポインタ取得 (mutable)
    __forceinline wchar_t* DataMut() noexcept
    {
        return IsSSO() ? m_sso.buffer : m_heap.data;
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return DataMut(); }
    __forceinline const_iterator begin()  const noexcept { return Data(); }
    __forceinline const_iterator cbegin() const noexcept { return Data(); }
    __forceinline iterator       end()          noexcept { return DataMut() + Size(); }
    __forceinline const_iterator end()    const noexcept { return Data() + Size(); }
    __forceinline const_iterator cend()   const noexcept { return Data() + Size(); }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 文字列長 (wchar_t単位)
    __forceinline size_type Size() const noexcept
    {
        return IsSSO() ? GetSSOSize() : m_heap.size;
    }

    /// @brief 文字列長 (Size のエイリアス)
    __forceinline size_type Length() const noexcept { return Size(); }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return Size() == 0; }

    /// @brief 現在のキャパシティ (wchar_t単位、null終端を含まない)
    __forceinline size_type Capacity() const noexcept
    {
        return IsSSO() ? k_SSOCapacity : GetHeapCapacity();
    }

    // ========================================================================
    // Reserve
    // ========================================================================

    /// @brief キャパシティを拡張
    void Reserve(size_type newCap)
    {
        if (newCap <= Capacity())
            return;

        if (IsSSO())
        {
            size_type oldSize = GetSSOSize();
            wchar_t tmpBuf[k_SSOCapacity + 1];
            std::memcpy(tmpBuf, m_sso.buffer, (oldSize + 1) * sizeof(wchar_t));

            DoAllocateAndInit(newCap);
            std::memcpy(m_heap.data, tmpBuf, (oldSize + 1) * sizeof(wchar_t));
            m_heap.size = oldSize;
        }
        else
        {
            size_type oldSize = m_heap.size;
            wchar_t* oldData = m_heap.data;
            size_type oldCapBytes = (GetHeapCapacity() + 1) * sizeof(wchar_t);

            m_heap.data = static_cast<wchar_t*>(
                GetAllocator().allocate((newCap + 1) * sizeof(wchar_t), alignof(wchar_t), 0, 0));
            m_heap.capacity = newCap;
            MarkHeap();
            std::memcpy(m_heap.data, oldData, (oldSize + 1) * sizeof(wchar_t));
            GetAllocator().deallocate(oldData, oldCapBytes);
        }
        SyncDebugVars();
    }

    /// @brief キャパシティをサイズに縮小
    void ShrinkToFit()
    {
        if (!IsSSO())
        {
            size_type sz = m_heap.size;
            if (sz <= k_SSOCapacity)
            {
                wchar_t* oldData = m_heap.data;
                size_type oldCapBytes = (GetHeapCapacity() + 1) * sizeof(wchar_t);
                std::memcpy(m_sso.buffer, oldData, (sz + 1) * sizeof(wchar_t));
                SetSSOSize(sz);
                GetAllocator().deallocate(oldData, oldCapBytes);
            }
            else if (GetHeapCapacity() > sz)
            {
                wchar_t* oldData = m_heap.data;
                size_type oldCapBytes = (GetHeapCapacity() + 1) * sizeof(wchar_t);
                m_heap.data = static_cast<wchar_t*>(
                    GetAllocator().allocate((sz + 1) * sizeof(wchar_t), alignof(wchar_t), 0, 0));
                m_heap.capacity = sz;
                MarkHeap();
                std::memcpy(m_heap.data, oldData, (sz + 1) * sizeof(wchar_t));
                GetAllocator().deallocate(oldData, oldCapBytes);
            }
        }
        SyncDebugVars();
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 末尾に1文字追加
    void push_back(wchar_t c)
    {
        size_type oldSize = Size();
        size_type newSize = oldSize + 1;
        if (newSize > Capacity())
            DoGrow(newSize);
        wchar_t* dst = DataMut();
        dst[oldSize] = c;
        dst[newSize] = L'\0';
        SetSize(newSize);
        SyncDebugVars();
    }

    /// @brief 末尾に追加
    WString& append(const wchar_t* str, size_type len)
    {
        if (len == 0)
            return *this;

        size_type oldSize = Size();
        size_type newSize = oldSize + len;

        if (newSize > Capacity())
            DoGrow(newSize);

        wchar_t* dst = DataMut();
        std::memcpy(dst + oldSize, str, len * sizeof(wchar_t));
        dst[newSize] = L'\0';
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 末尾にワイド文字列を追加
    WString& append(const wchar_t* str)
    {
        return append(str, std::wcslen(str));
    }

    /// @brief 末尾にWStringを追加
    WString& append(const WString& other)
    {
        return append(other.Data(), other.Size());
    }

    WString& operator+=(const WString& other)  { return append(other); }
    WString& operator+=(const wchar_t* str)     { return append(str); }
    WString& operator+=(wchar_t c)
    {
        size_type oldSize = Size();
        size_type newSize = oldSize + 1;
        if (newSize > Capacity())
            DoGrow(newSize);
        wchar_t* dst = DataMut();
        dst[oldSize] = c;
        dst[newSize] = L'\0';
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 全文字を削除
    void clear() noexcept
    {
        if (IsSSO())
        {
            m_sso.buffer[0] = L'\0';
            SetSSOSize(0);
        }
        else
        {
            m_heap.data[0] = L'\0';
            m_heap.size = 0;
        }
        SyncDebugVars();
    }

    /// @brief サイズ変更
    void resize(size_type n, wchar_t c = L'\0')
    {
        size_type oldSize = Size();
        if (n > Capacity())
            DoGrow(n);

        wchar_t* dst = DataMut();
        if (n > oldSize)
        {
            for (size_type i = oldSize; i < n; ++i)
                dst[i] = c;
        }
        dst[n] = L'\0';
        SetSize(n);
        SyncDebugVars();
    }

    // ========================================================================
    // Search
    // ========================================================================

    /// @brief 前方検索
    size_type find(const wchar_t* str, size_type pos = 0) const noexcept
    {
        if (!str) return npos;
        size_type sz = Size();
        size_type strLen = std::wcslen(str);
        if (strLen == 0) return (pos <= sz) ? pos : npos;
        if (pos + strLen > sz) return npos;
        const wchar_t* src = Data();
        for (size_type i = pos; i + strLen <= sz; ++i)
        {
            if (std::wmemcmp(src + i, str, strLen) == 0)
                return i;
        }
        return npos;
    }

    size_type find(wchar_t c, size_type pos = 0) const noexcept
    {
        const wchar_t* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
            if (src[i] == c) return i;
        return npos;
    }

    size_type find(const WString& str, size_type pos = 0) const noexcept
    {
        return find(str.c_str(), pos);
    }

    /// @brief 後方検索
    size_type rfind(const wchar_t* str, size_type pos = npos) const noexcept
    {
        if (!str) return npos;
        size_type sz = Size();
        size_type strLen = std::wcslen(str);
        if (strLen == 0) return (pos >= sz) ? sz : pos;
        if (strLen > sz) return npos;
        const wchar_t* src = Data();
        size_type startPos = sz - strLen;
        if (pos < startPos) startPos = pos;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::wmemcmp(src + i - 1, str, strLen) == 0)
                return i - 1;
        }
        return npos;
    }

    size_type rfind(wchar_t c, size_type pos = npos) const noexcept
    {
        const wchar_t* src = Data();
        size_type sz = Size();
        if (sz == 0) return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
            if (src[i - 1] == c) return i - 1;
        return npos;
    }

    /// @brief 文字集合の前方検索
    size_type find_first_of(const wchar_t* chars, size_type pos = 0) const noexcept
    {
        if (!chars) return npos;
        const wchar_t* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
        {
            if (std::wcschr(chars, src[i]) != nullptr)
                return i;
        }
        return npos;
    }

    size_type find_first_of(wchar_t c, size_type pos = 0) const noexcept
    {
        return find(c, pos);
    }

    /// @brief 文字集合の後方検索
    size_type find_last_of(const wchar_t* chars, size_type pos = npos) const noexcept
    {
        if (!chars) return npos;
        const wchar_t* src = Data();
        size_type sz = Size();
        if (sz == 0) return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::wcschr(chars, src[i - 1]) != nullptr)
                return i - 1;
        }
        return npos;
    }

    size_type find_last_of(wchar_t c, size_type pos = npos) const noexcept
    {
        return rfind(c, pos);
    }

    /// @brief 文字集合以外の前方検索
    size_type find_first_not_of(const wchar_t* chars, size_type pos = 0) const noexcept
    {
        if (!chars) return (pos < Size()) ? pos : npos;
        const wchar_t* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
        {
            if (std::wcschr(chars, src[i]) == nullptr)
                return i;
        }
        return npos;
    }

    /// @brief 文字集合以外の後方検索
    size_type find_last_not_of(const wchar_t* chars, size_type pos = npos) const noexcept
    {
        if (!chars) { size_type sz = Size(); return (sz > 0) ? sz - 1 : npos; }
        const wchar_t* src = Data();
        size_type sz = Size();
        if (sz == 0) return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::wcschr(chars, src[i - 1]) == nullptr)
                return i - 1;
        }
        return npos;
    }

    // ========================================================================
    // Substring
    // ========================================================================

    WString substr(size_type pos = 0, size_type count = npos) const
    {
        size_type sz = Size();
        assert(pos <= sz);
        if (count == npos || pos + count > sz)
            count = sz - pos;
        return WString(Data() + pos, count);
    }

    // ========================================================================
    // Insert / Erase / Replace
    // ========================================================================

    WString& insert(size_type pos, const wchar_t* str, size_type len)
    {
        assert(pos <= Size());
        if (len == 0) return *this;
        size_type oldSize = Size();
        size_type newSize = oldSize + len;
        if (newSize > Capacity()) DoGrow(newSize);
        wchar_t* dst = DataMut();
        std::memmove(dst + pos + len, dst + pos, (oldSize - pos + 1) * sizeof(wchar_t));
        std::memcpy(dst + pos, str, len * sizeof(wchar_t));
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    WString& insert(size_type pos, const wchar_t* str)
    {
        return insert(pos, str, std::wcslen(str));
    }

    WString& insert(size_type pos, const WString& other)
    {
        return insert(pos, other.Data(), other.Size());
    }

    WString& erase(size_type pos = 0, size_type count = npos)
    {
        size_type sz = Size();
        assert(pos <= sz);
        if (count == npos || pos + count > sz) count = sz - pos;
        if (count == 0) return *this;
        wchar_t* dst = DataMut();
        size_type tailLen = sz - pos - count;
        std::memmove(dst + pos, dst + pos + count, (tailLen + 1) * sizeof(wchar_t));
        SetSize(sz - count);
        SyncDebugVars();
        return *this;
    }

    WString& replace(size_type pos, size_type count, const wchar_t* str, size_type len)
    {
        assert(pos <= Size());
        size_type sz = Size();
        if (pos + count > sz) count = sz - pos;
        size_type newSize = sz - count + len;
        if (newSize > Capacity()) DoGrow(newSize);
        wchar_t* dst = DataMut();
        size_type tailLen = sz - pos - count;
        std::memmove(dst + pos + len, dst + pos + count, (tailLen + 1) * sizeof(wchar_t));
        std::memcpy(dst + pos, str, len * sizeof(wchar_t));
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    WString& replace(size_type pos, size_type count, const wchar_t* str)
    {
        return replace(pos, count, str, std::wcslen(str));
    }

    // ========================================================================
    // Compare
    // ========================================================================

    int compare(const WString& other) const noexcept
    {
        size_type lhs = Size(), rhs = other.Size();
        size_type minLen = (lhs < rhs) ? lhs : rhs;
        int cmp = std::wmemcmp(Data(), other.Data(), minLen);
        if (cmp != 0) return cmp;
        if (lhs < rhs) return -1;
        if (lhs > rhs) return 1;
        return 0;
    }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline size_type size() const noexcept { return Size(); }
    __forceinline size_type length() const noexcept { return Length(); }
    __forceinline bool empty() const noexcept { return Empty(); }
    __forceinline size_type capacity() const noexcept { return Capacity(); }
    void reserve(size_type n) { Reserve(n); }
    void shrink_to_fit() { ShrinkToFit(); }
    __forceinline reference front() noexcept { return Front(); }
    __forceinline const_reference front() const noexcept { return Front(); }
    __forceinline reference back() noexcept { return Back(); }
    __forceinline const_reference back() const noexcept { return Back(); }
    __forceinline pointer data() noexcept { return DataMut(); }
    __forceinline const_pointer data() const noexcept { return Data(); }
    __forceinline reference at(size_type i) { assert(i < Size()); return DataMut()[i]; }
    __forceinline const_reference at(size_type i) const { assert(i < Size()); return Data()[i]; }
    void pop_back() { assert(!Empty()); size_type ns = Size() - 1; DataMut()[ns] = L'\0'; SetSize(ns); SyncDebugVars(); }
    void assign(const wchar_t* str) { *this = WString(str); }
    void assign(const wchar_t* str, size_type len) { *this = WString(str, len); }
    void swap(WString& other) noexcept { Swap(other); }

    // ========================================================================
    // Swap
    // ========================================================================

    /// @brief スワップ
    void Swap(WString& other) noexcept
    {
        char tmp[sizeof(WString)];
        std::memcpy(tmp, this, sizeof(WString));
        std::memcpy(this, &other, sizeof(WString));
        std::memcpy(&other, tmp, sizeof(WString));
    }

    // ========================================================================
    // Conversion
    // ========================================================================

    /// @brief std::wstring に変換
    std::wstring ToStdWString() const
    {
        return std::wstring(Data(), Size());
    }

    /// @brief std::wstring_view に変換
    std::wstring_view ToWStringView() const noexcept
    {
        return std::wstring_view(Data(), Size());
    }

    /// @brief 暗黙変換 to std::wstring (std library boundary interop)
    operator std::wstring() const
    {
        return std::wstring(Data(), Size());
    }

    /// @brief 暗黙変換 to wstring_view
    operator std::wstring_view() const noexcept
    {
        return ToWStringView();
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const WString& a, const WString& b) noexcept
    {
        size_type sz = a.Size();
        if (sz != b.Size()) return false;
        return std::memcmp(a.Data(), b.Data(), sz * sizeof(wchar_t)) == 0;
    }

    friend bool operator!=(const WString& a, const WString& b) noexcept { return !(a == b); }

    friend bool operator<(const WString& a, const WString& b) noexcept
    {
        size_type minLen = (a.Size() < b.Size()) ? a.Size() : b.Size();
        int cmp = std::wmemcmp(a.Data(), b.Data(), minLen);
        if (cmp != 0) return cmp < 0;
        return a.Size() < b.Size();
    }

    friend bool operator>(const WString& a, const WString& b) noexcept  { return b < a; }
    friend bool operator<=(const WString& a, const WString& b) noexcept { return !(b < a); }
    friend bool operator>=(const WString& a, const WString& b) noexcept { return !(a < b); }

    // WString vs const wchar_t*
    friend bool operator==(const WString& a, const wchar_t* b) noexcept
    {
        if (!b) return a.Empty();
        return std::wcscmp(a.c_str(), b) == 0;
    }
    friend bool operator==(const wchar_t* a, const WString& b) noexcept { return b == a; }
    friend bool operator!=(const WString& a, const wchar_t* b) noexcept { return !(a == b); }
    friend bool operator!=(const wchar_t* a, const WString& b) noexcept { return !(b == a); }

    // ========================================================================
    // Concatenation
    // ========================================================================

    friend WString operator+(const WString& a, const WString& b)
    {
        WString result(a);
        result.append(b);
        return result;
    }

    friend WString operator+(const WString& a, const wchar_t* b)
    {
        WString result(a);
        if (b) result.append(b);
        return result;
    }

    friend WString operator+(const wchar_t* a, const WString& b)
    {
        WString result(a);
        result.append(b);
        return result;
    }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態を検証
    bool Validate() const noexcept
    {
        if (IsSSO())
            return GetSSOSize() <= k_SSOCapacity;
        else
            return m_heap.data != nullptr && m_heap.size <= GetHeapCapacity();
    }

private:
    // ========================================================================
    // SSO layout
    // ========================================================================

    /// @brief SSO レイアウト
    struct SSOLayout
    {
        wchar_t buffer[k_SSOCapacity + 1]; ///< インラインバッファ (11 wchar_t = 22 bytes)
        uint8_t ssoSizeFlag;               ///< SSO長 + SSO判定フラグ (bit7 = 0 => SSO)
        uint8_t padding;                   ///< アライメントパディング
    };

    /// @brief ヒープレイアウト
    struct HeapLayout
    {
        wchar_t*  data;      ///< ヒープバッファポインタ (8 bytes)
        size_type size;      ///< 文字列長 (8 bytes)
        size_type capacity;  ///< バッファ容量 (null終端を含まない) (8 bytes)
    };

    union
    {
        SSOLayout  m_sso;
        HeapLayout m_heap;
    };

    CompressedPair<Allocator, char> m_allocatorTag;

    size_type m_dbgSize = 0;       ///< [Debug] Size() のキャッシュ（デバッガ表示用）
    size_type m_dbgCapacity = 0;   ///< [Debug] Capacity() のキャッシュ（デバッガ表示用）

    /// @brief デバッグ変数を同期
    __forceinline void SyncDebugVars() noexcept
    {
        m_dbgSize = Size();
        m_dbgCapacity = Capacity();
    }

    // ========================================================================
    // SSO helpers
    // ========================================================================

    __forceinline bool IsSSO() const noexcept
    {
        return (m_sso.ssoSizeFlag & 0x80) == 0;
    }

    __forceinline size_type GetSSOSize() const noexcept
    {
        return static_cast<size_type>(m_sso.ssoSizeFlag & 0x7F);
    }

    __forceinline void SetSSOSize(size_type n) noexcept
    {
        assert(n <= k_SSOCapacity);
        m_sso.ssoSizeFlag = static_cast<uint8_t>(n & 0x7F);
    }

    __forceinline void MarkHeap() noexcept
    {
        m_sso.ssoSizeFlag |= 0x80;
    }

    /// @brief ヒープキャパシティ取得 (フラグビットをマスク)
    /// ssoSizeFlag (byte 22) overlaps with bit 55 of m_heap.capacity on little-endian
    __forceinline size_type GetHeapCapacity() const noexcept
    {
        return m_heap.capacity & ~(size_type(1) << 55);
    }

    __forceinline void SetSize(size_type n) noexcept
    {
        if (IsSSO())
            SetSSOSize(n);
        else
            m_heap.size = n;
    }

    __forceinline Allocator& GetAllocator() noexcept
    {
        return m_allocatorTag.First();
    }

    // ========================================================================
    // Internal helpers
    // ========================================================================

    void DoConstruct(const wchar_t* str, size_type len)
    {
        if (len <= k_SSOCapacity)
        {
            std::memcpy(m_sso.buffer, str, len * sizeof(wchar_t));
            m_sso.buffer[len] = L'\0';
            SetSSOSize(len);
        }
        else
        {
            DoAllocateAndInit(len);
            std::memcpy(m_heap.data, str, len * sizeof(wchar_t));
            m_heap.data[len] = L'\0';
            m_heap.size = len;
        }
    }

    void DoAllocateAndInit(size_type cap)
    {
        m_heap.data = static_cast<wchar_t*>(
            GetAllocator().allocate((cap + 1) * sizeof(wchar_t), alignof(wchar_t), 0, 0));
        m_heap.size = 0;
        m_heap.capacity = cap;
        MarkHeap();
    }

    void DoFree()
    {
        if (m_heap.data)
        {
            GetAllocator().deallocate(m_heap.data, (GetHeapCapacity() + 1) * sizeof(wchar_t));
            m_heap.data = nullptr;
        }
    }

    void DoGrow(size_type minCap)
    {
        size_type oldCap = Capacity();
        size_type newCap = (oldCap > 0) ? (oldCap * 2) : 16;
        if (newCap < minCap)
            newCap = minCap;

        size_type oldSize = Size();

        if (IsSSO())
        {
            wchar_t tmpBuf[k_SSOCapacity + 1];
            std::memcpy(tmpBuf, m_sso.buffer, (oldSize + 1) * sizeof(wchar_t));

            DoAllocateAndInit(newCap);
            std::memcpy(m_heap.data, tmpBuf, (oldSize + 1) * sizeof(wchar_t));
            m_heap.size = oldSize;
        }
        else
        {
            wchar_t* oldData = m_heap.data;
            size_type oldCapBytes = (GetHeapCapacity() + 1) * sizeof(wchar_t);

            m_heap.data = static_cast<wchar_t*>(
                GetAllocator().allocate((newCap + 1) * sizeof(wchar_t), alignof(wchar_t), 0, 0));
            m_heap.capacity = newCap;
            MarkHeap();
            std::memcpy(m_heap.data, oldData, (oldSize + 1) * sizeof(wchar_t));
            m_heap.size = oldSize;
            GetAllocator().deallocate(oldData, oldCapBytes);
        }
    }
};

/// @brief swap (ADL用)
inline void swap(WString& a, WString& b) noexcept
{
    a.Swap(b);
}

// ============================================================================
// Hash specialization
// ============================================================================

template <>
struct Hash<WString>
{
    __forceinline size_t operator()(const WString& s) const noexcept
    {
        return FNV1a(s.Data(), s.Size() * sizeof(wchar_t));
    }
};

} // namespace gx::container

// std::hash specialization for gx::container::WString
namespace std {
template <>
struct hash<gx::container::WString>
{
    size_t operator()(const gx::container::WString& s) const noexcept
    {
        return gx::container::FNV1a(s.Data(), s.Size() * sizeof(wchar_t));
    }
};
} // namespace std
