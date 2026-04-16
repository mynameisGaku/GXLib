/// @file test_AsyncLoadPipeline.cpp
/// @brief AsyncLoadPipeline テスト — 2スレッド非同期ロードパイプライン

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/AsyncLoadPipeline.h"
#include "IO/FileSystem.h"
#include "IO/PhysicalFileProvider.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

// ============================================================================
// テストフィクスチャ
// ============================================================================

class AsyncLoadPipelineTest : public ::testing::Test
{
protected:
    gx::String m_tempDir;
    std::shared_ptr<gx::PhysicalFileProvider> m_provider;

    void SetUp() override
    {
        char tmpBuf[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpBuf);
        m_tempDir = gx::String(tmpBuf) + "gxlib_async_test/";
        std::error_code ec;
        fs::remove_all(fs::path(m_tempDir.c_str()), ec);
        fs::create_directories(fs::path(m_tempDir.c_str()));

        // VFSにマウント
        m_provider = std::make_shared<gx::PhysicalFileProvider>(m_tempDir);
        gx::FileSystem::Instance().Mount("test", m_provider);
    }

    void TearDown() override
    {
        gx::FileSystem::Instance().Unmount("test");
        std::error_code ec;
        fs::remove_all(fs::path(m_tempDir.c_str()), ec);
    }

    void CreateTestFile(const gx::String& name, const gx::String& content)
    {
        gx::String path = m_tempDir + name;
        std::ofstream f(path.c_str());
        f << content;
    }

    // パイプラインの処理完了を待つヘルパー
    void WaitForCompletion(gx::AsyncLoadPipeline& pipeline, uint32_t taskId,
                           int maxIterations = 200)
    {
        for (int i = 0; i < maxIterations; ++i)
        {
            pipeline.Update();
            auto status = pipeline.GetStatus(taskId);
            if (status == gx::PipelineLoadStatus::Complete ||
                status == gx::PipelineLoadStatus::Error ||
                status == gx::PipelineLoadStatus::Cancelled)
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void WaitForAllCompletion(gx::AsyncLoadPipeline& pipeline,
                              const gx::Vector<uint32_t>& taskIds,
                              int maxIterations = 200)
    {
        for (int i = 0; i < maxIterations; ++i)
        {
            pipeline.Update();
            bool allDone = true;
            for (uint32_t id : taskIds)
            {
                auto status = pipeline.GetStatus(id);
                if (status != gx::PipelineLoadStatus::Complete &&
                    status != gx::PipelineLoadStatus::Error &&
                    status != gx::PipelineLoadStatus::Cancelled)
                {
                    allDone = false;
                    break;
                }
            }
            if (allDone) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

// ============================================================================
// 基本テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, SingleFileLoad)
{
    CreateTestFile("hello.txt", "Hello World");

    gx::AsyncLoadPipeline pipeline;
    bool integrated = false;
    gx::String parsedContent;

    uint32_t id = pipeline.Submit(
        "test/hello.txt",
        [](const gx::FileData& data) -> std::shared_ptr<void> {
            auto str = std::make_shared<gx::String>(
                reinterpret_cast<const char*>(data.Data()), data.Size());
            return str;
        },
        [&](std::shared_ptr<void> result) {
            parsedContent = *std::static_pointer_cast<gx::String>(result);
            integrated = true;
        });

    EXPECT_GT(id, 0u);

    WaitForCompletion(pipeline, id);
    EXPECT_TRUE(integrated);
    EXPECT_EQ(parsedContent, gx::String("Hello World"));
    EXPECT_EQ(pipeline.GetStatus(id), gx::PipelineLoadStatus::Complete);
}

TEST_F(AsyncLoadPipelineTest, MultipleLoads)
{
    CreateTestFile("a.txt", "AAA");
    CreateTestFile("b.txt", "BBB");
    CreateTestFile("c.txt", "CCC");

    gx::AsyncLoadPipeline pipeline;
    int completeCount = 0;

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<gx::String>(
            reinterpret_cast<const char*>(data.Data()), data.Size());
    };
    auto integrate = [&](std::shared_ptr<void>) { ++completeCount; };

    uint32_t id1 = pipeline.Submit("test/a.txt", parse, integrate);
    uint32_t id2 = pipeline.Submit("test/b.txt", parse, integrate);
    uint32_t id3 = pipeline.Submit("test/c.txt", parse, integrate);

    gx::Vector<uint32_t> ids;
    ids.push_back(id1);
    ids.push_back(id2);
    ids.push_back(id3);
    WaitForAllCompletion(pipeline, ids);

    EXPECT_EQ(completeCount, 3);
}

// ============================================================================
// 優先度テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, PriorityOrdering)
{
    // 複数ファイルを作成
    for (int i = 0; i < 5; ++i)
    {
        char name[32];
        snprintf(name, sizeof(name), "file%d.txt", i);
        char content[32];
        snprintf(content, sizeof(content), "data%d", i);
        CreateTestFile(gx::String(name), gx::String(content));
    }

    gx::AsyncLoadPipeline pipeline;
    gx::Vector<int> loadOrder;
    std::mutex orderMutex;

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    gx::Vector<uint32_t> ids;
    // Low priority first, then Critical
    for (int i = 0; i < 5; ++i)
    {
        char name[32];
        snprintf(name, sizeof(name), "test/file%d.txt", i);
        auto integrate = [&, i](std::shared_ptr<void>) {
            std::lock_guard<std::mutex> lock(orderMutex);
            loadOrder.push_back(i);
        };
        auto prio = (i == 4) ? gx::LoadPriority::Critical : gx::LoadPriority::Low;
        ids.push_back(pipeline.Submit(gx::String(name), parse, integrate, prio));
    }

    WaitForAllCompletion(pipeline, ids);

    // 全て完了することを確認
    EXPECT_EQ(static_cast<int>(loadOrder.size()), 5);
}

// ============================================================================
// 依存チェーンテスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, DependencyChain)
{
    CreateTestFile("step1.txt", "step1");
    CreateTestFile("step2.txt", "step2");

    gx::AsyncLoadPipeline pipeline;
    gx::Vector<int> order;
    std::mutex orderMutex;

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    uint32_t id1 = pipeline.Submit("test/step1.txt", parse,
        [&](std::shared_ptr<void>) {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(1);
        });

    uint32_t id2 = pipeline.SubmitAfter(id1, "test/step2.txt", parse,
        [&](std::shared_ptr<void>) {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(2);
        });

    WaitForCompletion(pipeline, id2);

    EXPECT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

// ============================================================================
// バッチテスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, BatchAllComplete)
{
    CreateTestFile("batch1.txt", "data1");
    CreateTestFile("batch2.txt", "data2");
    CreateTestFile("batch3.txt", "data3");

    gx::AsyncLoadPipeline pipeline;
    bool batchComplete = false;

    gx::Vector<gx::String> paths;
    paths.push_back("test/batch1.txt");
    paths.push_back("test/batch2.txt");
    paths.push_back("test/batch3.txt");

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    auto batch = pipeline.SubmitBatch(paths, parse,
        [&]() { batchComplete = true; });

    EXPECT_TRUE(batch.IsValid());

    // バッチ完了を待つ
    for (int i = 0; i < 200 && !batchComplete; ++i)
    {
        pipeline.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(batchComplete);
}

TEST_F(AsyncLoadPipelineTest, BatchProgress)
{
    CreateTestFile("p1.txt", "data1");
    CreateTestFile("p2.txt", "data2");

    gx::AsyncLoadPipeline pipeline;

    gx::Vector<gx::String> paths;
    paths.push_back("test/p1.txt");
    paths.push_back("test/p2.txt");

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    auto batch = pipeline.SubmitBatch(paths, parse, nullptr);

    // 完了を待つ
    for (int i = 0; i < 200; ++i)
    {
        pipeline.Update();
        float progress = pipeline.GetProgress(batch);
        if (progress >= 1.0f) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FLOAT_EQ(pipeline.GetProgress(batch), 1.0f);
}

TEST_F(AsyncLoadPipelineTest, EmptyBatch)
{
    gx::AsyncLoadPipeline pipeline;
    gx::Vector<gx::String> empty;
    auto batch = pipeline.SubmitBatch(empty, nullptr, nullptr);
    EXPECT_FALSE(batch.IsValid());
}

// ============================================================================
// Cancel テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, CancelTask)
{
    CreateTestFile("cancel.txt", "data");

    gx::AsyncLoadPipeline pipeline;

    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        // 時間のかかるparse
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return std::make_shared<int>(0);
    };

    uint32_t id = pipeline.Submit("test/cancel.txt", parse, nullptr);
    pipeline.Cancel(id);

    // 完了を待つ
    for (int i = 0; i < 50; ++i)
    {
        pipeline.Update();
        auto status = pipeline.GetStatus(id);
        if (status == gx::PipelineLoadStatus::Cancelled ||
            status == gx::PipelineLoadStatus::Complete)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto status = pipeline.GetStatus(id);
    EXPECT_TRUE(status == gx::PipelineLoadStatus::Cancelled ||
                status == gx::PipelineLoadStatus::Complete);
}

TEST_F(AsyncLoadPipelineTest, CancelAll)
{
    CreateTestFile("ca1.txt", "data");
    CreateTestFile("ca2.txt", "data");

    gx::AsyncLoadPipeline pipeline;
    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    pipeline.Submit("test/ca1.txt", parse, nullptr);
    pipeline.Submit("test/ca2.txt", parse, nullptr);
    pipeline.CancelAll();

    pipeline.Update();
    EXPECT_EQ(pipeline.GetActiveCount(), 0u);
}

// ============================================================================
// エラー処理テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, FileNotFoundError)
{
    gx::AsyncLoadPipeline pipeline;

    uint32_t id = pipeline.Submit("test/nonexistent.txt",
        [](const gx::FileData& data) -> std::shared_ptr<void> {
            return std::make_shared<int>(0);
        },
        nullptr);

    WaitForCompletion(pipeline, id);
    EXPECT_EQ(pipeline.GetStatus(id), gx::PipelineLoadStatus::Error);
}

TEST_F(AsyncLoadPipelineTest, ParseException)
{
    CreateTestFile("bad.txt", "bad data");

    gx::AsyncLoadPipeline pipeline;

    uint32_t id = pipeline.Submit("test/bad.txt",
        [](const gx::FileData& data) -> std::shared_ptr<void> {
            throw std::runtime_error("parse error");
            return nullptr;
        },
        nullptr);

    WaitForCompletion(pipeline, id);
    EXPECT_EQ(pipeline.GetStatus(id), gx::PipelineLoadStatus::Error);
}

// ============================================================================
// GetActiveCount テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, ActiveCountZeroAfterCompletion)
{
    CreateTestFile("active.txt", "data");

    gx::AsyncLoadPipeline pipeline;
    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    uint32_t id = pipeline.Submit("test/active.txt", parse, nullptr);
    WaitForCompletion(pipeline, id);
    EXPECT_EQ(pipeline.GetActiveCount(), 0u);
}

// ============================================================================
// NullFunc テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, NullParseFuncSkipsParse)
{
    CreateTestFile("null.txt", "null data");

    gx::AsyncLoadPipeline pipeline;
    bool integrated = false;

    uint32_t id = pipeline.Submit("test/null.txt",
        nullptr,  // no parse
        [&](std::shared_ptr<void>) { integrated = true; });

    WaitForCompletion(pipeline, id);
    EXPECT_TRUE(integrated);
    EXPECT_EQ(pipeline.GetStatus(id), gx::PipelineLoadStatus::Complete);
}

// ============================================================================
// GetStatus 未知ID テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, GetStatusUnknownId)
{
    gx::AsyncLoadPipeline pipeline;
    EXPECT_EQ(pipeline.GetStatus(99999), gx::PipelineLoadStatus::Error);
}

// ============================================================================
// BatchHandle テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, BatchHandleDefault)
{
    gx::BatchHandle h;
    EXPECT_FALSE(h.IsValid());
    EXPECT_EQ(h.id, 0u);
}

// ============================================================================
// GetProgress 無効バッチ
// ============================================================================

TEST_F(AsyncLoadPipelineTest, GetProgressInvalidBatch)
{
    gx::AsyncLoadPipeline pipeline;
    gx::BatchHandle invalid;
    EXPECT_FLOAT_EQ(pipeline.GetProgress(invalid), 0.0f);
}

// ============================================================================
// SubmitAfterWithCompletedDep テスト
// ============================================================================

TEST_F(AsyncLoadPipelineTest, SubmitAfterAlreadyCompleted)
{
    CreateTestFile("dep1.txt", "data1");
    CreateTestFile("dep2.txt", "data2");

    gx::AsyncLoadPipeline pipeline;
    auto parse = [](const gx::FileData& data) -> std::shared_ptr<void> {
        return std::make_shared<int>(0);
    };

    // 先行タスクを投入して完了を待つ
    uint32_t id1 = pipeline.Submit("test/dep1.txt", parse, nullptr);
    WaitForCompletion(pipeline, id1);
    EXPECT_EQ(pipeline.GetStatus(id1), gx::PipelineLoadStatus::Complete);

    // 既に完了した先行に依存するタスクを投入
    bool integrated = false;
    uint32_t id2 = pipeline.SubmitAfter(id1, "test/dep2.txt", parse,
        [&](std::shared_ptr<void>) { integrated = true; });

    WaitForCompletion(pipeline, id2);
    EXPECT_TRUE(integrated);
    EXPECT_EQ(pipeline.GetStatus(id2), gx::PipelineLoadStatus::Complete);
}
