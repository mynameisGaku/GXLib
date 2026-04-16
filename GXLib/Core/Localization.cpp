/// @file Localization.cpp
/// @brief 多言語対応システムの実装
#include "pch_common.h"
#include "Core/Localization.h"
#include "Core/Logger.h"

namespace gx
{

const gx::String Localization::s_emptyString;

Localization& Localization::Instance()
{
    static Localization instance;
    return instance;
}

bool Localization::LoadLanguage(const gx::String& language, const gx::String& filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        GX_LOG_ERROR("Localization: Failed to open '%s'", filePath.c_str());
        return false;
    }

    auto& strings = m_strings[language];
    gx::String line;
    while (gx::container::getline(file, line))
    {
        // 空行とコメント行をスキップ
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eq = line.find('=');
        if (eq == gx::String::npos) continue;

        gx::String key = line.substr(0, eq);
        gx::String value = line.substr(eq + 1);

        // キーの前後の空白を除去
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!value.empty() && value.front() == ' ') value.erase(0, 1);

        // \n のアンエスケープ
        for (size_t pos = 0; (pos = value.find("\\n", pos)) != gx::String::npos; pos += 1)
            value.replace(pos, 2, "\n");

        strings[key] = value;
    }

    GX_LOG_INFO("Localization: Loaded '%s' with %zu strings", language.c_str(), strings.size());
    return true;
}

const gx::String& Localization::GetString(const gx::String& key) const
{
    // 現在の言語で検索
    const auto& str = LookupString(m_currentLanguage, key);
    if (&str != &s_emptyString) return str;

    // フォールバック言語で検索
    if (m_fallbackLanguage != m_currentLanguage)
    {
        const auto& fallback = LookupString(m_fallbackLanguage, key);
        if (&fallback != &s_emptyString) return fallback;
    }

    // 最終手段としてキー自体を返す
    return key;
}

gx::Vector<gx::String> Localization::GetAvailableLanguages() const
{
    gx::Vector<gx::String> langs;
    langs.reserve(m_strings.size());
    for (const auto& [lang, _] : m_strings)
        langs.push_back(lang);
    return langs;
}

void Localization::Clear()
{
    m_strings.clear();
}

const gx::String& Localization::LookupString(const gx::String& lang, const gx::String& key) const
{
    auto langIt = m_strings.find(lang);
    if (langIt == m_strings.end()) return s_emptyString;
    auto keyIt = langIt->second.find(key);
    if (keyIt == langIt->second.end()) return s_emptyString;
    return keyIt->second;
}

} // namespace gx
