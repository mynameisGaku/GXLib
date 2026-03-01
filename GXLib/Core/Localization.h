#pragma once
/// @file Localization.h
/// @brief 多言語対応（i18n）システム
///
/// 言語ごとのキー=値ファイルを読み込み、GetString(key) で
/// 現在の言語に応じた文字列を返す。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief 多言語対応マネージャー（シングルトン）
class Localization
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return Localizationインスタンスへの参照
    static Localization& Instance();

    /// @brief 言語データをファイルから読み込む
    /// @param language 言語識別子（"ja", "en" 等）
    /// @param filePath データファイルパス（key=value形式）
    /// @return 成功でtrue
    bool LoadLanguage(const std::string& language, const std::string& filePath);

    /// @brief 現在の言語を設定する
    /// @param language 言語識別子
    void SetLanguage(const std::string& language) { m_currentLanguage = language; }

    /// @brief フォールバック言語を設定する（キーが見つからない場合に使う）
    /// @param language フォールバック言語識別子
    void SetFallbackLanguage(const std::string& language) { m_fallbackLanguage = language; }

    /// @brief 現在の言語を取得する
    /// @return 現在の言語識別子
    const std::string& GetLanguage() const { return m_currentLanguage; }

    /// @brief キーに対応するローカライズ文字列を取得する
    /// @param key 文字列キー
    /// @return ローカライズされた文字列（未定義の場合キー自体を返す）
    const std::string& GetString(const std::string& key) const;

    /// @brief フォーマット付きローカライズ文字列を取得する
    /// @param key 文字列キー
    /// @param args フォーマット引数
    /// @return フォーマット済みのローカライズ文字列
    template<typename... Args>
    std::string Format(const std::string& key, Args&&... args) const
    {
        const std::string& str = GetString(key);
        try { return std::vformat(str, std::make_format_args(args...)); }
        catch (...) { return str; }
    }

    /// @brief 利用可能な言語リストを取得する
    /// @return 読み込み済み言語識別子の配列
    std::vector<std::string> GetAvailableLanguages() const;

    /// @brief 全データをクリアする
    void Clear();

private:
    Localization() = default;

    const std::string& LookupString(const std::string& lang, const std::string& key) const;

    std::string m_currentLanguage  = "en";  ///< 現在の言語識別子
    std::string m_fallbackLanguage = "en"; ///< フォールバック言語識別子

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_strings; ///< 多言語文字列テーブル（language -> key -> value）
    static const std::string s_emptyString;  ///< キー未定義時に返す空文字列
};

} // namespace gx

/// @brief ローカライズ文字列取得マクロ
#define GX_LOC(key) gx::Localization::Instance().GetString(key)
/// @}
