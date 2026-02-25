/// @file test_FileSystem.cpp
/// @brief VFS FileSystem tests with mock IFileProvider

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/FileSystem.h"

using namespace GX;

/// Test in-memory file provider
class MockFileProvider : public IFileProvider
{
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    int m_priority;

    MockFileProvider(int priority = 0) : m_priority(priority) {}

    void AddFile(const std::string& path, const std::string& content)
    {
        files[path] = std::vector<uint8_t>(content.begin(), content.end());
    }

    bool Exists(const std::string& path) const override
    {
        return files.count(path) > 0;
    }

    FileData Read(const std::string& path) const override
    {
        FileData fd;
        auto it = files.find(path);
        if (it != files.end()) fd.data = it->second;
        return fd;
    }

    bool Write(const std::string& path, const void* data, size_t size) override
    {
        files[path] = std::vector<uint8_t>(
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
    std::string content = "Hello World";
    fd.data = std::vector<uint8_t>(content.begin(), content.end());
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
    std::string content = "written data";
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
