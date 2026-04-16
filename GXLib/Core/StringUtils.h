#pragma once
/// @file StringUtils.h
/// @brief 文字列ユーティリティ関数群（トリム、分割、結合、変換等）
///
/// gx::String / gx::WString に対する汎用操作を提供する静的クラス。
/// ヘッダーオンリーで全関数インライン定義。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx {

/// @brief 文字列操作ユーティリティを提供する静的クラス
class StringUtils
{
public:
    StringUtils() = delete;

    // ========================================================================
    // トリム
    // ========================================================================

    /// @brief 先頭と末尾の空白文字を除去する
    /// @param s 入力文字列
    /// @return トリム済み文字列
    static inline gx::String Trim(const gx::String& s)
    {
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == gx::String::npos) return {};
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        return s.substr(start, end - start + 1);
    }

    /// @brief 先頭の空白文字を除去する
    /// @param s 入力文字列
    /// @return 先頭トリム済み文字列
    static inline gx::String TrimLeft(const gx::String& s)
    {
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == gx::String::npos) return {};
        return s.substr(start);
    }

    /// @brief 末尾の空白文字を除去する
    /// @param s 入力文字列
    /// @return 末尾トリム済み文字列
    static inline gx::String TrimRight(const gx::String& s)
    {
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        if (end == gx::String::npos) return {};
        return s.substr(0, end + 1);
    }

    // ========================================================================
    // 大文字・小文字変換
    // ========================================================================

    /// @brief 全文字を小文字に変換する
    /// @param s 入力文字列
    /// @return 小文字化された文字列
    static inline gx::String ToLower(const gx::String& s)
    {
        gx::String result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    /// @brief 全文字を大文字に変換する
    /// @param s 入力文字列
    /// @return 大文字化された文字列
    static inline gx::String ToUpper(const gx::String& s)
    {
        gx::String result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return result;
    }

    // ========================================================================
    // 分割・結合
    // ========================================================================

    /// @brief 文字で分割する
    /// @param s 入力文字列
    /// @param delimiter 区切り文字
    /// @return 分割された文字列のベクター
    static inline gx::Vector<gx::String> Split(const gx::String& s, char delimiter)
    {
        gx::Vector<gx::String> tokens;
        size_t start = 0;
        size_t end   = s.find(delimiter);

        while (end != gx::String::npos)
        {
            tokens.push_back(s.substr(start, end - start));
            start = end + 1;
            end   = s.find(delimiter, start);
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }

    /// @brief 文字列で分割する
    /// @param s 入力文字列
    /// @param delimiter 区切り文字列
    /// @return 分割された文字列のベクター
    static inline gx::Vector<gx::String> Split(const gx::String& s, const gx::String& delimiter)
    {
        gx::Vector<gx::String> tokens;
        if (delimiter.empty())
        {
            tokens.push_back(s);
            return tokens;
        }

        size_t start = 0;
        size_t end   = s.find(delimiter);

        while (end != gx::String::npos)
        {
            tokens.push_back(s.substr(start, end - start));
            start = end + delimiter.length();
            end   = s.find(delimiter, start);
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }

    /// @brief 文字列ベクターをセパレータで結合する
    /// @param parts 結合対象の文字列ベクター
    /// @param separator 区切り文字列
    /// @return 結合された文字列
    static inline gx::String Join(const gx::Vector<gx::String>& parts, const gx::String& separator)
    {
        if (parts.empty()) return {};

        gx::String result = parts[0];
        for (size_t i = 1; i < parts.size(); ++i)
        {
            result += separator;
            result += parts[i];
        }
        return result;
    }

    // ========================================================================
    // 検索・判定
    // ========================================================================

    /// @brief 指定プレフィックスで始まるか判定する
    /// @param s 入力文字列
    /// @param prefix プレフィックス
    /// @return プレフィックスで始まる場合 true
    static inline bool StartsWith(const gx::String& s, const gx::String& prefix)
    {
        if (prefix.size() > s.size()) return false;
        return s.compare(0, prefix.size(), prefix) == 0;
    }

    /// @brief 指定サフィックスで終わるか判定する
    /// @param s 入力文字列
    /// @param suffix サフィックス
    /// @return サフィックスで終わる場合 true
    static inline bool EndsWith(const gx::String& s, const gx::String& suffix)
    {
        if (suffix.size() > s.size()) return false;
        return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    /// @brief 部分文字列を含むか判定する
    /// @param s 入力文字列
    /// @param substr 検索する部分文字列
    /// @return 含む場合 true
    static inline bool Contains(const gx::String& s, const gx::String& substr)
    {
        return s.find(substr) != gx::String::npos;
    }

    // ========================================================================
    // 置換
    // ========================================================================

    /// @brief 全ての出現箇所を置換する
    /// @param s 入力文字列
    /// @param from 検索文字列
    /// @param to 置換文字列
    /// @return 置換後の文字列
    static inline gx::String Replace(const gx::String& s, const gx::String& from, const gx::String& to)
    {
        if (from.empty()) return s;

        gx::String result = s;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != gx::String::npos)
        {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }

    // ========================================================================
    // 文字コード変換
    // ========================================================================

    /// @brief ナロー文字列（ANSI/UTF-8）からワイド文字列に変換する
    /// @param s ナロー文字列
    /// @return ワイド文字列
    static inline gx::WString ToWide(const gx::String& s)
    {
        if (s.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        if (size <= 0) return {};
        gx::WString result(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), size);
        return result;
    }

    /// @brief ワイド文字列からナロー文字列（UTF-8）に変換する
    /// @param s ワイド文字列
    /// @return ナロー文字列（UTF-8）
    static inline gx::String ToNarrow(const gx::WString& s)
    {
        if (s.empty()) return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        gx::String result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), size, nullptr, nullptr);
        return result;
    }
};

} // namespace gx
/// @}
