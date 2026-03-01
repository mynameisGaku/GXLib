/// @file test_Localization.cpp
/// @brief Localizationクラスのユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Localization.h"
#include <fstream>
#include <cstdio>

using namespace gx;

class LocalizationFullTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Localization::Instance().Clear();
        Localization::Instance().SetLanguage("en");
        Localization::Instance().SetFallbackLanguage("en");
    }

    void TearDown() override
    {
        Localization::Instance().Clear();
        // 一時ファイルを削除
        for (auto& path : m_tempFiles)
            std::remove(path.c_str());
    }

    std::string CreateTempFile(const std::string& content)
    {
        std::string path = std::tmpnam(nullptr);
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        m_tempFiles.push_back(path);
        return path;
    }

    std::vector<std::string> m_tempFiles;
};

TEST_F(LocalizationFullTest, DefaultLanguageIsEn)
{
    EXPECT_EQ(Localization::Instance().GetLanguage(), "en");
}

TEST_F(LocalizationFullTest, SetLanguageAndGet)
{
    Localization::Instance().SetLanguage("ja");
    EXPECT_EQ(Localization::Instance().GetLanguage(), "ja");
}

TEST_F(LocalizationFullTest, SetFallbackLanguage)
{
    Localization::Instance().SetFallbackLanguage("fr");
    // フォールバックは現在の言語を変更しない
    EXPECT_EQ(Localization::Instance().GetLanguage(), "en");
}

TEST_F(LocalizationFullTest, GetStringReturnsKeyWhenNotLoaded)
{
    EXPECT_EQ(Localization::Instance().GetString("unknown_key"), "unknown_key");
}

TEST_F(LocalizationFullTest, LoadLanguageFromFile)
{
    std::string path = CreateTempFile("greeting=Hello\nfarewell=Goodbye\n");
    bool ok = Localization::Instance().LoadLanguage("en", path);
    EXPECT_TRUE(ok);
}

TEST_F(LocalizationFullTest, GetStringReturnsCorrectValue)
{
    std::string path = CreateTempFile("greeting=Hello\nfarewell=Goodbye\n");
    Localization::Instance().LoadLanguage("en", path);

    EXPECT_EQ(Localization::Instance().GetString("greeting"), "Hello");
    EXPECT_EQ(Localization::Instance().GetString("farewell"), "Goodbye");
}

TEST_F(LocalizationFullTest, FallbackLanguageUsedWhenKeyMissing)
{
    std::string enPath = CreateTempFile("title=My Game\nsubtitle=Adventure\n");
    std::string jaPath = CreateTempFile("title=My Game JP\n");

    Localization::Instance().LoadLanguage("en", enPath);
    Localization::Instance().LoadLanguage("ja", jaPath);
    Localization::Instance().SetLanguage("ja");
    Localization::Instance().SetFallbackLanguage("en");

    // "title"はjaに存在する
    EXPECT_EQ(Localization::Instance().GetString("title"), "My Game JP");
    // "subtitle"はjaに存在しないのでenにフォールバック
    EXPECT_EQ(Localization::Instance().GetString("subtitle"), "Adventure");
}

TEST_F(LocalizationFullTest, GetAvailableLanguages)
{
    std::string enPath = CreateTempFile("a=b\n");
    std::string jaPath = CreateTempFile("c=d\n");

    Localization::Instance().LoadLanguage("en", enPath);
    Localization::Instance().LoadLanguage("ja", jaPath);

    auto langs = Localization::Instance().GetAvailableLanguages();
    EXPECT_EQ(langs.size(), 2u);

    bool hasEn = false, hasJa = false;
    for (const auto& l : langs)
    {
        if (l == "en") hasEn = true;
        if (l == "ja") hasJa = true;
    }
    EXPECT_TRUE(hasEn);
    EXPECT_TRUE(hasJa);
}

TEST_F(LocalizationFullTest, ClearRemovesAllData)
{
    std::string path = CreateTempFile("key=value\n");
    Localization::Instance().LoadLanguage("en", path);
    EXPECT_EQ(Localization::Instance().GetString("key"), "value");

    Localization::Instance().Clear();
    // クリア後、キーはそのまま返される
    EXPECT_EQ(Localization::Instance().GetString("key"), "key");
    EXPECT_TRUE(Localization::Instance().GetAvailableLanguages().empty());
}

TEST_F(LocalizationFullTest, MultipleLanguagesSimultaneously)
{
    std::string enPath = CreateTempFile("hello=Hello\n");
    std::string frPath = CreateTempFile("hello=Bonjour\n");
    std::string dePath = CreateTempFile("hello=Hallo\n");

    Localization::Instance().LoadLanguage("en", enPath);
    Localization::Instance().LoadLanguage("fr", frPath);
    Localization::Instance().LoadLanguage("de", dePath);

    Localization::Instance().SetLanguage("en");
    EXPECT_EQ(Localization::Instance().GetString("hello"), "Hello");

    Localization::Instance().SetLanguage("fr");
    EXPECT_EQ(Localization::Instance().GetString("hello"), "Bonjour");

    Localization::Instance().SetLanguage("de");
    EXPECT_EQ(Localization::Instance().GetString("hello"), "Hallo");
}

TEST_F(LocalizationFullTest, CommentLinesAndEmptyLinesSkipped)
{
    std::string path = CreateTempFile(
        "# This is a comment\n"
        "\n"
        "; Another comment\n"
        "key1=value1\n"
        "\n"
        "key2=value2\n"
    );
    Localization::Instance().LoadLanguage("en", path);

    EXPECT_EQ(Localization::Instance().GetString("key1"), "value1");
    EXPECT_EQ(Localization::Instance().GetString("key2"), "value2");
}

TEST_F(LocalizationFullTest, NewlineEscapeUnescaped)
{
    std::string path = CreateTempFile("msg=Line1\\nLine2\n");
    Localization::Instance().LoadLanguage("en", path);

    EXPECT_EQ(Localization::Instance().GetString("msg"), "Line1\nLine2");
}

TEST_F(LocalizationFullTest, LoadLanguageFailsForInvalidPath)
{
    bool ok = Localization::Instance().LoadLanguage("en", "/nonexistent/path/file.txt");
    EXPECT_FALSE(ok);
}
