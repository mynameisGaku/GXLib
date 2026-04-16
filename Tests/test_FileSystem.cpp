/// @file test_FileSystem.cpp
/// @brief モックIFileProviderを使ったVFS FileSystemのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/FileSystem.h"

using namespace gx;

/// テスト用インメモリファイルプロバイダー
class MockFileProvider : public IFileProvider
{
public:
    gx::HashMap<gx::String, gx::Vector<uint8_t>> files;
    int m_priority;

    MockFileProvider(int priority = 0) : m_priority(priority) {}

    void AddFile(const gx::String& path, const gx::String& content)
    {
        files[path] = gx::Vector<uint8_t>(content.begin(), content.end());
    }

    bool Exists(const gx::String& path) const override
    {
        return files.count(path) > 0;
    }

    FileData Read(const gx::String& path) const override
    {
        FileData fd;
        auto it = files.find(path);
        if (it != files.end()) fd.data = it->second;
        return fd;
    }

    bool Write(const gx::String& path, const void* data, size_t size) override
    {
        files[path] = gx::Vector<uint8_t>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size);
        return true;
    }

    int Priority() const override { return m_priority; }
};

class FileSystemTest : public ::testing::Test
{
protected:
    void SetUp() override { FileSystem::Instance().Clear(); }
    void TearDown() override { FileSystem::Instance().Clear(); }
};

TEST(FileDataTest, Empty)
{
    FileData fd;
    EXPECT_FALSE(fd.IsValid());
}

TEST(FileDataTest, AsString)
{
    FileData fd;
    gx::String content = "Hello World";
    fd.data = gx::Vector<uint8_t>(content.begin(), content.end());
    EXPECT_TRUE(fd.IsValid());
    EXPECT_EQ(fd.AsString(), "Hello World");
}

TEST_F(FileSystemTest, Mount_Exists)
{
    auto provider = std::make_shared<MockFileProvider>();
    provider->AddFile("test.txt", "content");
    FileSystem::Instance().Mount("", provider);
    EXPECT_TRUE(FileSystem::Instance().Exists("test.txt"));
}

TEST_F(FileSystemTest, Mount_ReadFile)
{
    auto provider = std::make_shared<MockFileProvider>();
    provider->AddFile("data.txt", "hello");
    FileSystem::Instance().Mount("", provider);
    FileData fd = FileSystem::Instance().ReadFile("data.txt");
    EXPECT_TRUE(fd.IsValid());
    EXPECT_EQ(fd.AsString(), "hello");
}

TEST_F(FileSystemTest, Mount_WriteFile)
{
    auto provider = std::make_shared<MockFileProvider>();
    FileSystem::Instance().Mount("", provider);
    gx::String content = "written data";
    FileSystem::Instance().WriteFile("out.txt", content.data(), content.size());
    FileData fd = FileSystem::Instance().ReadFile("out.txt");
    EXPECT_EQ(fd.AsString(), "written data");
}

TEST_F(FileSystemTest, Mount_NotFound)
{
    EXPECT_FALSE(FileSystem::Instance().Exists("nonexistent.txt"));
}

TEST_F(FileSystemTest, Mount_MountPoint)
{
    auto provider = std::make_shared<MockFileProvider>();
    provider->AddFile("test.txt", "content");
    FileSystem::Instance().Mount("assets/", provider);
    EXPECT_TRUE(FileSystem::Instance().Exists("assets/test.txt"));
}

TEST_F(FileSystemTest, Priority_HigherWins)
{
    auto lowPri = std::make_shared<MockFileProvider>(0);
    lowPri->AddFile("data.txt", "low");
    auto highPri = std::make_shared<MockFileProvider>(10);
    highPri->AddFile("data.txt", "high");
    FileSystem::Instance().Mount("", lowPri);
    FileSystem::Instance().Mount("", highPri);
    FileData fd = FileSystem::Instance().ReadFile("data.txt");
    EXPECT_EQ(fd.AsString(), "high");
}

TEST_F(FileSystemTest, Unmount_Removes)
{
    auto provider = std::make_shared<MockFileProvider>();
    provider->AddFile("test.txt", "content");
    FileSystem::Instance().Mount("mnt/", provider);
    EXPECT_TRUE(FileSystem::Instance().Exists("mnt/test.txt"));
    FileSystem::Instance().Unmount("mnt/");
    EXPECT_FALSE(FileSystem::Instance().Exists("mnt/test.txt"));
}

TEST_F(FileSystemTest, Clear_RemovesAll)
{
    auto p1 = std::make_shared<MockFileProvider>();
    p1->AddFile("a.txt", "a");
    auto p2 = std::make_shared<MockFileProvider>();
    p2->AddFile("b.txt", "b");
    FileSystem::Instance().Mount("", p1);
    FileSystem::Instance().Mount("extra/", p2);
    FileSystem::Instance().Clear();
    EXPECT_FALSE(FileSystem::Instance().Exists("a.txt"));
    EXPECT_FALSE(FileSystem::Instance().Exists("extra/b.txt"));
}

// ============================================================================
// 追加テスト: FileData
// ============================================================================

TEST(FileDataTest, DataPointer)
{
    FileData fd;
    gx::String content = "binary data";
    fd.data = gx::Vector<uint8_t>(content.begin(), content.end());
    EXPECT_NE(fd.Data(), nullptr);
    EXPECT_EQ(fd.Size(), content.size());
}

TEST(FileDataTest, SizeOfEmptyData)
{
    FileData fd;
    EXPECT_EQ(fd.Size(), 0u);
    EXPECT_EQ(fd.Data(), nullptr);
}

TEST(FileDataTest, AsStringPreservesContent)
{
    FileData fd;
    gx::String content = "line1\nline2\nline3";
    fd.data = gx::Vector<uint8_t>(content.begin(), content.end());
    EXPECT_EQ(fd.AsString(), content);
}

// ============================================================================
// 追加テスト: Mount & Read
// ============================================================================

TEST_F(FileSystemTest, ReadFileNotFound)
{
    auto provider = std::make_shared<MockFileProvider>();
    FileSystem::Instance().Mount("", provider);

    FileData fd = FileSystem::Instance().ReadFile("nonexistent.txt");
    EXPECT_FALSE(fd.IsValid());
}

TEST_F(FileSystemTest, WriteAndReadBack)
{
    auto provider = std::make_shared<MockFileProvider>();
    FileSystem::Instance().Mount("", provider);

    gx::String content = "round trip test data";
    FileSystem::Instance().WriteFile("roundtrip.txt", content.data(), content.size());

    FileData fd = FileSystem::Instance().ReadFile("roundtrip.txt");
    EXPECT_TRUE(fd.IsValid());
    EXPECT_EQ(fd.AsString(), content);
}

TEST_F(FileSystemTest, OverwriteExistingFile)
{
    auto provider = std::make_shared<MockFileProvider>();
    provider->AddFile("data.txt", "original");
    FileSystem::Instance().Mount("", provider);

    gx::String newContent = "updated";
    FileSystem::Instance().WriteFile("data.txt", newContent.data(), newContent.size());

    FileData fd = FileSystem::Instance().ReadFile("data.txt");
    EXPECT_EQ(fd.AsString(), "updated");
}

TEST_F(FileSystemTest, MultipleMountPoints)
{
    auto assetsProvider = std::make_shared<MockFileProvider>();
    assetsProvider->AddFile("sprite.png", "png_data");
    auto audioProvider = std::make_shared<MockFileProvider>();
    audioProvider->AddFile("bgm.wav", "wav_data");

    FileSystem::Instance().Mount("assets/", assetsProvider);
    FileSystem::Instance().Mount("audio/", audioProvider);

    EXPECT_TRUE(FileSystem::Instance().Exists("assets/sprite.png"));
    EXPECT_TRUE(FileSystem::Instance().Exists("audio/bgm.wav"));
    EXPECT_FALSE(FileSystem::Instance().Exists("assets/bgm.wav"));
    EXPECT_FALSE(FileSystem::Instance().Exists("audio/sprite.png"));
}

TEST_F(FileSystemTest, UnmountOnlyTargetedMountPoint)
{
    auto p1 = std::make_shared<MockFileProvider>();
    p1->AddFile("a.txt", "a");
    auto p2 = std::make_shared<MockFileProvider>();
    p2->AddFile("b.txt", "b");

    FileSystem::Instance().Mount("m1/", p1);
    FileSystem::Instance().Mount("m2/", p2);

    FileSystem::Instance().Unmount("m1/");
    EXPECT_FALSE(FileSystem::Instance().Exists("m1/a.txt"));
    EXPECT_TRUE(FileSystem::Instance().Exists("m2/b.txt"));
}

TEST_F(FileSystemTest, EmptyFileWrite)
{
    auto provider = std::make_shared<MockFileProvider>();
    FileSystem::Instance().Mount("", provider);

    FileSystem::Instance().WriteFile("empty.txt", "", 0);
    FileData fd = FileSystem::Instance().ReadFile("empty.txt");
    // 空ファイルはIsValid()がfalseになる（空データ）
    EXPECT_FALSE(fd.IsValid());
    EXPECT_EQ(fd.Size(), 0u);
}

TEST_F(FileSystemTest, LargeFileContent)
{
    auto provider = std::make_shared<MockFileProvider>();
    FileSystem::Instance().Mount("", provider);

    // 大きなファイルコンテンツ
    gx::String largeContent(10000, 'X');
    FileSystem::Instance().WriteFile("large.bin", largeContent.data(), largeContent.size());

    FileData fd = FileSystem::Instance().ReadFile("large.bin");
    EXPECT_TRUE(fd.IsValid());
    EXPECT_EQ(fd.Size(), 10000u);
    EXPECT_EQ(fd.AsString(), largeContent);
}

TEST_F(FileSystemTest, Priority_LowerDoesNotOverride)
{
    auto highPri = std::make_shared<MockFileProvider>(10);
    highPri->AddFile("data.txt", "high");
    auto lowPri = std::make_shared<MockFileProvider>(0);
    lowPri->AddFile("data.txt", "low");

    // 高い優先度のものを先にマウントしても高い方が優先される
    FileSystem::Instance().Mount("", highPri);
    FileSystem::Instance().Mount("", lowPri);

    FileData fd = FileSystem::Instance().ReadFile("data.txt");
    EXPECT_EQ(fd.AsString(), "high");
}

TEST_F(FileSystemTest, SingletonConsistency)
{
    auto& fs1 = FileSystem::Instance();
    auto& fs2 = FileSystem::Instance();
    EXPECT_EQ(&fs1, &fs2);
}
