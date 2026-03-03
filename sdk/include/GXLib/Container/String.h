#pragma once
/// @file String.h
/// @brief SSO文字列 (22文字インライン、32バイト固定サイズ、ゲーム拡張付き)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/Hash.h"
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief SSO文字列クラス
///
/// Small String Optimization: 22文字以下はヒープ確保なしでインライン格納。
/// 32バイト固定サイズ (SSO時: char[23] + lengthバイト = 24, + padding/flags で32)。
/// ヒープ時はアロケータ経由で確保し、CompressedPairでアロケータを圧縮。
class String
{
public:
    // ========================================================================
    // Constants
    // ========================================================================

    using size_type       = size_t;
    using value_type      = char;
    using reference       = char&;
    using const_reference = const char&;
    using pointer         = char*;
    using const_pointer   = const char*;
    using iterator        = char*;
    using const_iterator  = const char*;
    using allocator_type  = Allocator;

    static constexpr size_type npos = static_cast<size_type>(-1);

    /// @brief SSO バッファサイズ (null終端分を含む内部バッファ)
    /// 実際の格納可能文字数は k_SSOCapacity = 22
    static constexpr size_type k_SSOCapacity = 22;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ (空文字列、SSO)
    String() noexcept
    {
        SetSSOSize(0);
        m_sso.buffer[0] = '\0';
        SyncDebugVars();
    }

    /// @brief C文字列から構築
    String(const char* str)
    {
        if (str)
        {
            size_type len = std::strlen(str);
            DoConstruct(str, len);
        }
        else
        {
            SetSSOSize(0);
            m_sso.buffer[0] = '\0';
        }
        SyncDebugVars();
    }

    /// @brief C文字列 + 長さから構築
    String(const char* str, size_type len)
    {
        DoConstruct(str, len);
        SyncDebugVars();
    }

    /// @brief n個のcharで構築
    String(size_type n, char c)
    {
        if (n <= k_SSOCapacity)
        {
            std::memset(m_sso.buffer, c, n);
            m_sso.buffer[n] = '\0';
            SetSSOSize(n);
        }
        else
        {
            DoAllocateAndInit(n);
            std::memset(m_heap.data, c, n);
            m_heap.data[n] = '\0';
            m_heap.size = n;
        }
        SyncDebugVars();
    }

    /// @brief std::string から構築
    String(const std::string& str)
    {
        DoConstruct(str.data(), str.size());
        SyncDebugVars();
    }

    /// @brief std::string_view から構築
    String(std::string_view sv)
    {
        DoConstruct(sv.data(), sv.size());
        SyncDebugVars();
    }

    /// @brief コピーコンストラクタ
    String(const String& other)
    {
        if (other.IsSSO())
        {
            std::memcpy(this, &other, sizeof(SSOLayout));
        }
        else
        {
            size_type len = other.Size();
            DoAllocateAndInit(len);
            std::memcpy(m_heap.data, other.m_heap.data, len + 1);
            m_heap.size = len;
        }
        SyncDebugVars();
    }

    /// @brief ムーブコンストラクタ
    String(String&& other) noexcept
    {
        std::memcpy(this, &other, sizeof(String));
        // 元をSSO空文字列にリセット
        other.SetSSOSize(0);
        other.m_sso.buffer[0] = '\0';
        other.SyncDebugVars();
    }

    /// @brief デストラクタ
    ~String()
    {
        if (!IsSSO())
            DoFree();
    }

    // ========================================================================
    // Assignment
    // ========================================================================

    /// @brief コピー代入
    String& operator=(const String& other)
    {
        if (this != &other)
        {
            String tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    /// @brief ムーブ代入
    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            if (!IsSSO())
                DoFree();
            std::memcpy(this, &other, sizeof(String));
            other.SetSSOSize(0);
            other.m_sso.buffer[0] = '\0';
            other.SyncDebugVars();
        }
        return *this;
    }

    /// @brief C文字列代入
    String& operator=(const char* str)
    {
        String tmp(str);
        Swap(tmp);
        return *this;
    }

    /// @brief std::string 代入
    String& operator=(const std::string& str)
    {
        String tmp(str.data(), str.size());
        Swap(tmp);
        return *this;
    }

    /// @brief std::string_view 代入
    String& operator=(std::string_view sv)
    {
        String tmp(sv);
        Swap(tmp);
        return *this;
    }

    /// @brief char 代入
    String& operator=(char c)
    {
        if (!IsSSO())
            DoFree();
        SetSSOSize(1);
        m_sso.buffer[0] = c;
        m_sso.buffer[1] = '\0';
        SyncDebugVars();
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
    __forceinline const char* c_str() const noexcept { return Data(); }

    /// @brief 生ポインタ取得
    __forceinline const char* Data() const noexcept
    {
        return IsSSO() ? m_sso.buffer : m_heap.data;
    }

    /// @brief 生ポインタ取得 (mutable)
    __forceinline char* DataMut() noexcept
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

    /// @brief 文字列長
    __forceinline size_type Size() const noexcept
    {
        return IsSSO() ? GetSSOSize() : m_heap.size;
    }

    /// @brief 文字列長 (Size のエイリアス)
    __forceinline size_type Length() const noexcept { return Size(); }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return Size() == 0; }

    /// @brief 現在のキャパシティ (null終端を含まない格納可能文字数)
    __forceinline size_type Capacity() const noexcept
    {
        // MarkHeap() sets bit 63 of capacity for SSO flag; mask it out
        return IsSSO() ? k_SSOCapacity : GetHeapCapacity();
    }

    /// @brief キャパシティを拡張
    void Reserve(size_type newCap)
    {
        if (newCap <= Capacity())
            return;

        if (IsSSO())
        {
            // SSO -> ヒープ遷移
            size_type oldSize = GetSSOSize();
            char tmpBuf[k_SSOCapacity + 1];
            std::memcpy(tmpBuf, m_sso.buffer, oldSize + 1);

            DoAllocateAndInit(newCap);
            std::memcpy(m_heap.data, tmpBuf, oldSize + 1);
            m_heap.size = oldSize;
        }
        else
        {
            // ヒープ再確保
            size_type oldSize = m_heap.size;
            char* oldData = m_heap.data;
            size_type oldCapBytes = (GetHeapCapacity() + 1);

            m_heap.data = static_cast<char*>(
                GetAllocator().allocate(newCap + 1, alignof(char), 0, 0));
            m_heap.capacity = newCap;
            MarkHeap();
            std::memcpy(m_heap.data, oldData, oldSize + 1);
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
                // ヒープ -> SSO 遷移
                char* oldData = m_heap.data;
                size_type oldCapBytes = GetHeapCapacity() + 1;
                std::memcpy(m_sso.buffer, oldData, sz + 1);
                SetSSOSize(sz);
                GetAllocator().deallocate(oldData, oldCapBytes);
            }
            else if (GetHeapCapacity() > sz)
            {
                // 縮小再確保
                char* oldData = m_heap.data;
                size_type oldCapBytes = GetHeapCapacity() + 1;

                m_heap.data = static_cast<char*>(
                    GetAllocator().allocate(sz + 1, alignof(char), 0, 0));
                m_heap.capacity = sz;
                MarkHeap();
                std::memcpy(m_heap.data, oldData, sz + 1);
                GetAllocator().deallocate(oldData, oldCapBytes);
            }
        }
        SyncDebugVars();
    }

    // ========================================================================
    // Modifiers — append / operator+=
    // ========================================================================

    /// @brief 末尾に文字列を追加
    String& append(const char* str, size_type len)
    {
        if (len == 0)
            return *this;

        size_type oldSize = Size();
        size_type newSize = oldSize + len;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        std::memcpy(dst + oldSize, str, len);
        dst[newSize] = '\0';
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 末尾にC文字列を追加
    String& append(const char* str)
    {
        return append(str, std::strlen(str));
    }

    /// @brief 末尾にStringを追加
    String& append(const String& other)
    {
        return append(other.Data(), other.Size());
    }

    /// @brief 末尾にstring_viewを追加
    String& append(std::string_view sv)
    {
        return append(sv.data(), sv.size());
    }

    /// @brief 末尾にn個のcharを追加
    String& append(size_type n, char c)
    {
        if (n == 0)
            return *this;

        size_type oldSize = Size();
        size_type newSize = oldSize + n;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        std::memset(dst + oldSize, c, n);
        dst[newSize] = '\0';
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 末尾に1文字追加
    void push_back(char c)
    {
        size_type oldSize = Size();
        size_type newSize = oldSize + 1;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        dst[oldSize] = c;
        dst[newSize] = '\0';
        SetSize(newSize);
        SyncDebugVars();
    }

    String& operator+=(const String& other)  { return append(other); }
    String& operator+=(const char* str)       { return append(str); }
    String& operator+=(std::string_view sv)   { return append(sv); }
    String& operator+=(char c)                { push_back(c); return *this; }

    // ========================================================================
    // Modifiers — insert / erase / replace
    // ========================================================================

    /// @brief 指定位置に文字列を挿入
    String& insert(size_type pos, const char* str, size_type len)
    {
        assert(pos <= Size());
        if (len == 0)
            return *this;

        size_type oldSize = Size();
        size_type newSize = oldSize + len;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        // 後方を移動
        std::memmove(dst + pos + len, dst + pos, oldSize - pos + 1); // +1 for null
        std::memcpy(dst + pos, str, len);
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 指定位置にC文字列を挿入
    String& insert(size_type pos, const char* str)
    {
        return insert(pos, str, std::strlen(str));
    }

    /// @brief 指定位置にn個のcharを挿入
    String& insert(size_type pos, size_type n, char c)
    {
        assert(pos <= Size());
        if (n == 0)
            return *this;

        size_type oldSize = Size();
        size_type newSize = oldSize + n;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        std::memmove(dst + pos + n, dst + pos, oldSize - pos + 1);
        std::memset(dst + pos, c, n);
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 範囲を削除
    String& erase(size_type pos = 0, size_type count = npos)
    {
        size_type sz = Size();
        assert(pos <= sz);

        if (count == npos || pos + count > sz)
            count = sz - pos;

        if (count == 0)
            return *this;

        char* dst = DataMut();
        size_type tailLen = sz - pos - count;
        std::memmove(dst + pos, dst + pos + count, tailLen + 1); // +1 for null
        SetSize(sz - count);
        SyncDebugVars();
        return *this;
    }

    /// @brief 部分文字列を置換
    String& replace(size_type pos, size_type count, const char* str, size_type len)
    {
        assert(pos <= Size());
        size_type sz = Size();
        if (pos + count > sz)
            count = sz - pos;

        size_type newSize = sz - count + len;

        if (newSize > Capacity())
            DoGrow(newSize);

        char* dst = DataMut();
        // 後方を移動
        size_type tailLen = sz - pos - count;
        std::memmove(dst + pos + len, dst + pos + count, tailLen + 1);
        std::memcpy(dst + pos, str, len);
        SetSize(newSize);
        SyncDebugVars();
        return *this;
    }

    /// @brief 部分文字列を置換
    String& replace(size_type pos, size_type count, const char* str)
    {
        return replace(pos, count, str, std::strlen(str));
    }

    /// @brief 部分文字列を String で置換
    String& replace(size_type pos, size_type count, const String& other)
    {
        return replace(pos, count, other.Data(), other.Size());
    }

    /// @brief 全文字を削除
    void clear() noexcept
    {
        if (IsSSO())
        {
            m_sso.buffer[0] = '\0';
            SetSSOSize(0);
        }
        else
        {
            m_heap.data[0] = '\0';
            m_heap.size = 0;
        }
        SyncDebugVars();
    }

    // ========================================================================
    // Substring
    // ========================================================================

    /// @brief 部分文字列を取得
    String substr(size_type pos = 0, size_type count = npos) const
    {
        size_type sz = Size();
        assert(pos <= sz);
        if (count == npos || pos + count > sz)
            count = sz - pos;
        return String(Data() + pos, count);
    }

    // ========================================================================
    // Search
    // ========================================================================

    /// @brief 前方検索
    size_type find(const char* str, size_type pos = 0) const noexcept
    {
        if (!str)
            return npos;
        size_type sz = Size();
        size_type strLen = std::strlen(str);
        if (strLen == 0)
            return (pos <= sz) ? pos : npos;
        if (pos + strLen > sz)
            return npos;

        const char* src = Data();
        for (size_type i = pos; i + strLen <= sz; ++i)
        {
            if (std::memcmp(src + i, str, strLen) == 0)
                return i;
        }
        return npos;
    }

    /// @brief 文字の前方検索
    size_type find(char c, size_type pos = 0) const noexcept
    {
        const char* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
        {
            if (src[i] == c)
                return i;
        }
        return npos;
    }

    /// @brief String の前方検索
    size_type find(const String& str, size_type pos = 0) const noexcept
    {
        return find(str.c_str(), pos);
    }

    /// @brief 後方検索
    size_type rfind(const char* str, size_type pos = npos) const noexcept
    {
        if (!str)
            return npos;
        size_type sz = Size();
        size_type strLen = std::strlen(str);
        if (strLen == 0)
            return (pos >= sz) ? sz : pos;
        if (strLen > sz)
            return npos;

        const char* src = Data();
        size_type startPos = sz - strLen;
        if (pos < startPos)
            startPos = pos;

        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::memcmp(src + i - 1, str, strLen) == 0)
                return i - 1;
        }
        return npos;
    }

    /// @brief 文字の後方検索
    size_type rfind(char c, size_type pos = npos) const noexcept
    {
        const char* src = Data();
        size_type sz = Size();
        if (sz == 0)
            return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (src[i - 1] == c)
                return i - 1;
        }
        return npos;
    }

    /// @brief 文字集合の前方検索
    size_type find_first_of(const char* chars, size_type pos = 0) const noexcept
    {
        if (!chars)
            return npos;
        const char* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
        {
            if (std::strchr(chars, src[i]) != nullptr)
                return i;
        }
        return npos;
    }

    /// @brief 文字集合の後方検索
    size_type find_last_of(const char* chars, size_type pos = npos) const noexcept
    {
        if (!chars)
            return npos;
        const char* src = Data();
        size_type sz = Size();
        if (sz == 0)
            return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::strchr(chars, src[i - 1]) != nullptr)
                return i - 1;
        }
        return npos;
    }

    /// @brief 文字集合以外の前方検索
    size_type find_first_not_of(const char* chars, size_type pos = 0) const noexcept
    {
        if (!chars)
            return (pos < Size()) ? pos : npos;
        const char* src = Data();
        size_type sz = Size();
        for (size_type i = pos; i < sz; ++i)
        {
            if (std::strchr(chars, src[i]) == nullptr)
                return i;
        }
        return npos;
    }

    /// @brief 文字集合以外の後方検索
    size_type find_last_not_of(const char* chars, size_type pos = npos) const noexcept
    {
        if (!chars)
        {
            size_type sz = Size();
            return (sz > 0) ? sz - 1 : npos;
        }
        const char* src = Data();
        size_type sz = Size();
        if (sz == 0)
            return npos;
        size_type startPos = (pos < sz) ? pos : sz - 1;
        for (size_type i = startPos + 1; i > 0; --i)
        {
            if (std::strchr(chars, src[i - 1]) == nullptr)
                return i - 1;
        }
        return npos;
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

    void pop_back()
    {
        assert(!Empty());
        size_type newSize = Size() - 1;
        DataMut()[newSize] = '\0';
        SetSize(newSize);
        SyncDebugVars();
    }

    void assign(const char* str) { *this = String(str); }
    void assign(const char* str, size_type len) { *this = String(str, len); }
    void assign(size_type n, char c) { *this = String(n, c); }
    void assign(const String& other) { *this = other; }

    int compare(const String& other) const noexcept
    {
        size_type lhs = Size(), rhs = other.Size();
        size_type minLen = (lhs < rhs) ? lhs : rhs;
        int cmp = std::memcmp(Data(), other.Data(), minLen);
        if (cmp != 0) return cmp;
        if (lhs < rhs) return -1;
        if (lhs > rhs) return 1;
        return 0;
    }

    int compare(const char* str) const noexcept
    {
        return std::strcmp(c_str(), str ? str : "");
    }

    int compare(size_type pos, size_type count, const char* str) const noexcept
    {
        size_type sz = Size();
        if (pos > sz) pos = sz;
        if (count == npos || pos + count > sz) count = sz - pos;
        size_type strLen = str ? std::strlen(str) : 0;
        size_type minLen = (count < strLen) ? count : strLen;
        int cmp = std::memcmp(Data() + pos, str ? str : "", minLen);
        if (cmp != 0) return cmp;
        if (count < strLen) return -1;
        if (count > strLen) return 1;
        return 0;
    }

    int compare(size_type pos, size_type count, const String& other) const noexcept
    {
        return compare(pos, count, other.c_str());
    }

    int compare(size_type pos, size_type count, const char* str, size_type strLen) const noexcept
    {
        size_type sz = Size();
        if (pos > sz) pos = sz;
        if (count == npos || pos + count > sz) count = sz - pos;
        size_type minLen = (count < strLen) ? count : strLen;
        int cmp = std::memcmp(Data() + pos, str, minLen);
        if (cmp != 0) return cmp;
        if (count < strLen) return -1;
        if (count > strLen) return 1;
        return 0;
    }

    bool starts_with(const char* prefix) const noexcept
    {
        if (!prefix) return true;
        size_type plen = std::strlen(prefix);
        return Size() >= plen && std::memcmp(Data(), prefix, plen) == 0;
    }

    bool starts_with(char c) const noexcept
    {
        return !Empty() && Data()[0] == c;
    }

    bool starts_with(std::string_view sv) const noexcept
    {
        return Size() >= sv.size() && std::memcmp(Data(), sv.data(), sv.size()) == 0;
    }

    bool ends_with(const char* suffix) const noexcept
    {
        if (!suffix) return true;
        size_type slen = std::strlen(suffix);
        return Size() >= slen && std::memcmp(Data() + Size() - slen, suffix, slen) == 0;
    }

    bool ends_with(char c) const noexcept
    {
        return !Empty() && Data()[Size() - 1] == c;
    }

    bool ends_with(std::string_view sv) const noexcept
    {
        return Size() >= sv.size() && std::memcmp(Data() + Size() - sv.size(), sv.data(), sv.size()) == 0;
    }

    void swap(String& other) noexcept { Swap(other); }

    /// @brief operator<< for ostream
    friend std::ostream& operator<<(std::ostream& os, const String& s)
    {
        os.write(s.Data(), static_cast<std::streamsize>(s.Size()));
        return os;
    }

    /// @brief operator>> for istream
    friend std::istream& operator>>(std::istream& is, String& s)
    {
        std::string tmp;
        is >> tmp;
        s = String(tmp.c_str(), tmp.size());
        return is;
    }

    // ========================================================================
    // Game extensions
    // ========================================================================

    /// @brief printf形式でフォーマット (既存内容を上書き)
    static String Sprintf(const char* fmt, ...)
    {
        String result;
        va_list args;
        va_start(args, fmt);

        va_list argsCopy;
        va_copy(argsCopy, args);
        int len = std::vsnprintf(nullptr, 0, fmt, argsCopy);
        va_end(argsCopy);

        if (len > 0)
        {
            result.resize(static_cast<size_type>(len));
            std::vsnprintf(result.DataMut(), static_cast<size_type>(len) + 1, fmt, args);
        }

        va_end(args);
        return result;
    }

    /// @brief printf形式で末尾に追加
    String& AppendSprintf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        va_list argsCopy;
        va_copy(argsCopy, args);
        int len = std::vsnprintf(nullptr, 0, fmt, argsCopy);
        va_end(argsCopy);

        if (len > 0)
        {
            size_type oldSize = Size();
            size_type newSize = oldSize + static_cast<size_type>(len);
            if (newSize > Capacity())
                DoGrow(newSize);
            std::vsnprintf(DataMut() + oldSize, static_cast<size_type>(len) + 1, fmt, args);
            SetSize(newSize);
        }

        va_end(args);
        SyncDebugVars();
        return *this;
    }

    /// @brief 大文字小文字無視比較 (0: equal, <0: less, >0: greater)
    int CompareI(const String& other) const noexcept
    {
        return CompareI(other.c_str());
    }

    /// @brief 大文字小文字無視比較 (C文字列)
    int CompareI(const char* str) const noexcept
    {
        if (!str)
            return (Size() > 0) ? 1 : 0;
        return _stricmp(c_str(), str);
    }

    /// @brief 前後の空白を除去
    String& Trim()
    {
        TrimLeft();
        TrimRight();
        return *this;
    }

    /// @brief 先頭の空白を除去
    String& TrimLeft()
    {
        const char* src = Data();
        size_type sz = Size();
        size_type i = 0;
        while (i < sz && std::isspace(static_cast<unsigned char>(src[i])))
            ++i;
        if (i > 0)
            erase(0, i);
        return *this;
    }

    /// @brief 末尾の空白を除去
    String& TrimRight()
    {
        size_type sz = Size();
        if (sz == 0)
            return *this;
        const char* src = Data();
        size_type i = sz;
        while (i > 0 && std::isspace(static_cast<unsigned char>(src[i - 1])))
            --i;
        if (i < sz)
            erase(i, sz - i);
        return *this;
    }

    /// @brief 全文字を小文字に変換
    String& ToLower()
    {
        char* dst = DataMut();
        size_type sz = Size();
        for (size_type i = 0; i < sz; ++i)
            dst[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(dst[i])));
        return *this;
    }

    /// @brief 全文字を大文字に変換
    String& ToUpper()
    {
        char* dst = DataMut();
        size_type sz = Size();
        for (size_type i = 0; i < sz; ++i)
            dst[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(dst[i])));
        return *this;
    }

    // ========================================================================
    // Conversion
    // ========================================================================

    /// @brief std::string に変換
    std::string ToStdString() const
    {
        return std::string(Data(), Size());
    }

    /// @brief std::string_view に変換
    std::string_view ToStringView() const noexcept
    {
        return std::string_view(Data(), Size());
    }

    /// @brief 暗黙変換 to std::string (std library boundary interop)
    operator std::string() const
    {
        return std::string(Data(), Size());
    }

    /// @brief 暗黙変換 to string_view
    operator std::string_view() const noexcept
    {
        return ToStringView();
    }

    // ========================================================================
    // Resize
    // ========================================================================

    /// @brief サイズを変更 (拡張部分は'\0'で埋める)
    void resize(size_type n)
    {
        resize(n, '\0');
    }

    /// @brief サイズを変更 (拡張部分は指定charで埋める)
    void resize(size_type n, char c)
    {
        size_type oldSize = Size();
        if (n > Capacity())
            DoGrow(n);

        char* dst = DataMut();
        if (n > oldSize)
            std::memset(dst + oldSize, c, n - oldSize);
        dst[n] = '\0';
        SetSize(n);
        SyncDebugVars();
    }

    // ========================================================================
    // Swap
    // ========================================================================

    /// @brief スワップ
    void Swap(String& other) noexcept
    {
        // 両方32バイト固定なのでバイトコピーでOK
        char tmp[sizeof(String)];
        std::memcpy(tmp, this, sizeof(String));
        std::memcpy(this, &other, sizeof(String));
        std::memcpy(&other, tmp, sizeof(String));
    }

    // ========================================================================
    // Comparison operators
    // ========================================================================

    friend bool operator==(const String& a, const String& b) noexcept
    {
        size_type sz = a.Size();
        if (sz != b.Size()) return false;
        return std::memcmp(a.Data(), b.Data(), sz) == 0;
    }

    friend bool operator!=(const String& a, const String& b) noexcept { return !(a == b); }

    friend bool operator<(const String& a, const String& b) noexcept
    {
        size_type minLen = (a.Size() < b.Size()) ? a.Size() : b.Size();
        int cmp = std::memcmp(a.Data(), b.Data(), minLen);
        if (cmp != 0) return cmp < 0;
        return a.Size() < b.Size();
    }

    friend bool operator>(const String& a, const String& b) noexcept  { return b < a; }
    friend bool operator<=(const String& a, const String& b) noexcept { return !(b < a); }
    friend bool operator>=(const String& a, const String& b) noexcept { return !(a < b); }

    // String vs const char*
    friend bool operator==(const String& a, const char* b) noexcept
    {
        if (!b) return a.Empty();
        return std::strcmp(a.c_str(), b) == 0;
    }
    friend bool operator==(const char* a, const String& b) noexcept { return b == a; }
    friend bool operator!=(const String& a, const char* b) noexcept { return !(a == b); }
    friend bool operator!=(const char* a, const String& b) noexcept { return !(b == a); }

    // String vs std::string_view
    friend bool operator==(const String& a, std::string_view b) noexcept
    {
        if (a.Size() != b.size()) return false;
        return std::memcmp(a.Data(), b.data(), a.Size()) == 0;
    }
    friend bool operator==(std::string_view a, const String& b) noexcept { return b == a; }
    friend bool operator!=(const String& a, std::string_view b) noexcept { return !(a == b); }
    friend bool operator!=(std::string_view a, const String& b) noexcept { return !(b == a); }

    // ========================================================================
    // Concatenation operators
    // ========================================================================

    friend String operator+(const String& a, const String& b)
    {
        String result;
        result.Reserve(a.Size() + b.Size());
        result.append(a);
        result.append(b);
        return result;
    }

    friend String operator+(const String& a, const char* b)
    {
        String result;
        size_type bLen = b ? std::strlen(b) : 0;
        result.Reserve(a.Size() + bLen);
        result.append(a);
        if (b) result.append(b, bLen);
        return result;
    }

    friend String operator+(const char* a, const String& b)
    {
        String result;
        size_type aLen = a ? std::strlen(a) : 0;
        result.Reserve(aLen + b.Size());
        if (a) result.append(a, aLen);
        result.append(b);
        return result;
    }

    friend String operator+(const String& a, char c)
    {
        String result(a);
        result.push_back(c);
        return result;
    }

    friend String operator+(char c, const String& b)
    {
        String result(1, c);
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
        {
            return GetSSOSize() <= k_SSOCapacity;
        }
        else
        {
            return m_heap.data != nullptr
                && m_heap.size <= GetHeapCapacity();
        }
    }

private:
    // ========================================================================
    // SSO layout
    // ========================================================================

    /// @brief SSO レイアウト (24バイト)
    struct SSOLayout
    {
        char    buffer[k_SSOCapacity + 1]; ///< インラインバッファ (23 bytes, null終端含む)
        uint8_t ssoSizeFlag;               ///< SSO長 + SSO判定フラグ (bit7 = 0 => SSO)
    };

    /// @brief ヒープレイアウト (24バイト + アロケータ)
    struct HeapLayout
    {
        char*     data;      ///< ヒープバッファポインタ (8 bytes)
        size_type size;      ///< 文字列長 (8 bytes)
        size_type capacity;  ///< バッファ容量 (null終端を含まない) (8 bytes)
    };

    // SSOとHeapはunion (同一メモリ)
    union
    {
        SSOLayout  m_sso;
        HeapLayout m_heap;
    };

    /// @brief CompressedPairでアロケータを圧縮
    /// ステートレスAllocatorならゼロバイト
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

    /// @brief SSO モードかどうか (最上位ビットが0ならSSO)
    __forceinline bool IsSSO() const noexcept
    {
        return (m_sso.ssoSizeFlag & 0x80) == 0;
    }

    /// @brief SSO サイズ取得
    __forceinline size_type GetSSOSize() const noexcept
    {
        return static_cast<size_type>(m_sso.ssoSizeFlag & 0x7F);
    }

    /// @brief SSO サイズ設定 (bit7 = 0 に保つ)
    __forceinline void SetSSOSize(size_type n) noexcept
    {
        assert(n <= k_SSOCapacity);
        m_sso.ssoSizeFlag = static_cast<uint8_t>(n & 0x7F);
    }

    /// @brief ヒープモードマーク設定
    __forceinline void MarkHeap() noexcept
    {
        m_sso.ssoSizeFlag |= 0x80;
    }

    /// @brief ヒープキャパシティ取得 (フラグビットをマスク)
    __forceinline size_type GetHeapCapacity() const noexcept
    {
        return m_heap.capacity & ~(size_type(1) << 63);
    }

    /// @brief サイズ設定 (SSO/ヒープ両対応)
    __forceinline void SetSize(size_type n) noexcept
    {
        if (IsSSO())
            SetSSOSize(n);
        else
            m_heap.size = n;
    }

    /// @brief アロケータ取得
    __forceinline Allocator& GetAllocator() noexcept
    {
        return m_allocatorTag.First();
    }

    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// @brief 文字列からの構築 (SSO or ヒープ自動判定)
    void DoConstruct(const char* str, size_type len)
    {
        if (len <= k_SSOCapacity)
        {
            std::memcpy(m_sso.buffer, str, len);
            m_sso.buffer[len] = '\0';
            SetSSOSize(len);
        }
        else
        {
            DoAllocateAndInit(len);
            std::memcpy(m_heap.data, str, len);
            m_heap.data[len] = '\0';
            m_heap.size = len;
        }
    }

    /// @brief ヒープバッファを確保して初期化
    void DoAllocateAndInit(size_type cap)
    {
        m_heap.data = static_cast<char*>(
            GetAllocator().allocate(cap + 1, alignof(char), 0, 0));
        m_heap.size = 0;
        m_heap.capacity = cap;
        MarkHeap();
    }

    /// @brief ヒープバッファ解放
    void DoFree()
    {
        if (m_heap.data)
        {
            GetAllocator().deallocate(m_heap.data, GetHeapCapacity() + 1);
            m_heap.data = nullptr;
        }
    }

    /// @brief 成長 (2x ポリシー)
    void DoGrow(size_type minCap)
    {
        size_type oldCap = Capacity();
        size_type newCap = (oldCap > 0) ? (oldCap * 2) : 32;
        if (newCap < minCap)
            newCap = minCap;

        size_type oldSize = Size();

        if (IsSSO())
        {
            // SSO -> ヒープ遷移
            char tmpBuf[k_SSOCapacity + 1];
            std::memcpy(tmpBuf, m_sso.buffer, oldSize + 1);

            DoAllocateAndInit(newCap);
            std::memcpy(m_heap.data, tmpBuf, oldSize + 1);
            m_heap.size = oldSize;
        }
        else
        {
            // ヒープ再確保
            char* oldData = m_heap.data;
            size_type oldCapBytes = GetHeapCapacity() + 1;

            m_heap.data = static_cast<char*>(
                GetAllocator().allocate(newCap + 1, alignof(char), 0, 0));
            m_heap.capacity = newCap;
            MarkHeap();
            std::memcpy(m_heap.data, oldData, oldSize + 1);
            m_heap.size = oldSize;
            GetAllocator().deallocate(oldData, oldCapBytes);
        }
    }
};

/// @brief swap (ADL用)
inline void swap(String& a, String& b) noexcept
{
    a.Swap(b);
}

// ============================================================================
// Hash specialization
// ============================================================================

template <>
struct Hash<String>
{
    __forceinline size_t operator()(const String& s) const noexcept
    {
        return FNV1a(s.Data(), s.Size());
    }
};

/// @brief getline for gx::container::String (std::getline interop)
inline std::istream& getline(std::istream& is, String& str, char delim = '\n')
{
    str.clear();
    char c;
    while (is.get(c))
    {
        if (c == delim) break;
        str.push_back(c);
    }
    // Match std::getline: if characters were extracted, clear failbit (keep eofbit)
    if (!str.empty() && is.eof() && !is.bad())
    {
        is.clear(is.rdstate() & ~std::ios_base::failbit);
    }
    return is;
}

} // namespace gx::container

// std::hash specialization for gx::container::String
namespace std {
template <>
struct hash<gx::container::String>
{
    size_t operator()(const gx::container::String& s) const noexcept
    {
        return gx::container::FNV1a(s.Data(), s.Size());
    }
};
} // namespace std
