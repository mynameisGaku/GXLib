/// @file test_ServiceLocator.cpp
/// @brief ServiceLocator + Engine + モック注入のテスト
#include <gtest/gtest.h>
#include "Core/ServiceLocator.h"
#include "Core/Engine.h"
#include "Audio/IAudioDevice.h"
#include "../Tests/Mocks/MockAudioDevice.h"

namespace {

/// @brief テスト用の汎用サービスインターフェース
class ITestService
{
public:
    virtual ~ITestService() = default;
    virtual int GetValue() const = 0;
};

/// @brief ITestServiceのモック実装
class MockTestService : public ITestService
{
public:
    explicit MockTestService(int value = 42) : m_value(value) {}
    int GetValue() const override { return m_value; }
private:
    int m_value;
};

class ServiceLocatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 各テスト前にServiceLocatorをクリア
        gx::ServiceLocator::Instance().Clear();
    }

    void TearDown() override
    {
        gx::ServiceLocator::Instance().Clear();
    }
};

TEST_F(ServiceLocatorTest, RegisterAndGet)
{
    auto mock = std::make_shared<gx::Testing::MockAudioDevice>();
    gx::ServiceLocator::Instance().Register<gx::IAudioDevice>(mock);

    auto& retrieved = gx::ServiceLocator::Instance().Get<gx::IAudioDevice>();
    EXPECT_EQ(&retrieved, mock.get());
}

TEST_F(ServiceLocatorTest, Has)
{
    EXPECT_FALSE(gx::ServiceLocator::Instance().Has<gx::IAudioDevice>());

    auto mock = std::make_shared<gx::Testing::MockAudioDevice>();
    gx::ServiceLocator::Instance().Register<gx::IAudioDevice>(mock);

    EXPECT_TRUE(gx::ServiceLocator::Instance().Has<gx::IAudioDevice>());
}

TEST_F(ServiceLocatorTest, GetThrowsWhenNotRegistered)
{
    EXPECT_THROW(
        gx::ServiceLocator::Instance().Get<gx::IAudioDevice>(),
        std::runtime_error
    );
}

TEST_F(ServiceLocatorTest, Clear)
{
    auto mock = std::make_shared<gx::Testing::MockAudioDevice>();
    gx::ServiceLocator::Instance().Register<gx::IAudioDevice>(mock);
    EXPECT_TRUE(gx::ServiceLocator::Instance().Has<gx::IAudioDevice>());

    gx::ServiceLocator::Instance().Clear();
    EXPECT_FALSE(gx::ServiceLocator::Instance().Has<gx::IAudioDevice>());
}

TEST_F(ServiceLocatorTest, EngineSetAndGetAudioDevice)
{
    auto mock = std::make_shared<gx::Testing::MockAudioDevice>();
    gx::Engine::Instance().SetAudioDevice(mock);

    auto& audio = gx::Engine::Instance().GetAudioDevice();
    audio.SetMasterVolume(0.5f);

    EXPECT_FLOAT_EQ(static_cast<gx::Testing::MockAudioDevice&>(audio).GetMasterVolume(), 0.5f);
}

TEST_F(ServiceLocatorTest, MockAudioDeviceLifecycle)
{
    gx::Testing::MockAudioDevice mock;
    EXPECT_FALSE(mock.IsInitialized());

    mock.Initialize();
    EXPECT_TRUE(mock.IsInitialized());
    EXPECT_EQ(mock.GetNativeEngine(), nullptr);
    EXPECT_EQ(mock.GetOutputChannelCount(), 2u);

    mock.SetMasterVolume(0.75f);
    EXPECT_FLOAT_EQ(mock.GetMasterVolume(), 0.75f);

    mock.Shutdown();
    EXPECT_FALSE(mock.IsInitialized());
}

TEST_F(ServiceLocatorTest, MultipleServicesCoexist)
{
    auto audio = std::make_shared<gx::Testing::MockAudioDevice>();
    auto testSvc = std::make_shared<MockTestService>(99);

    gx::ServiceLocator::Instance().Register<gx::IAudioDevice>(audio);
    gx::ServiceLocator::Instance().Register<ITestService>(testSvc);

    EXPECT_TRUE(gx::ServiceLocator::Instance().Has<gx::IAudioDevice>());
    EXPECT_TRUE(gx::ServiceLocator::Instance().Has<ITestService>());

    auto& a = gx::ServiceLocator::Instance().Get<gx::IAudioDevice>();
    auto& t = gx::ServiceLocator::Instance().Get<ITestService>();
    EXPECT_EQ(&a, audio.get());
    EXPECT_EQ(&t, testSvc.get());
    EXPECT_EQ(t.GetValue(), 99);
}

TEST_F(ServiceLocatorTest, RegisterOverwritesPrevious)
{
    auto svc1 = std::make_shared<MockTestService>(10);
    auto svc2 = std::make_shared<MockTestService>(20);

    gx::ServiceLocator::Instance().Register<ITestService>(svc1);
    EXPECT_EQ(gx::ServiceLocator::Instance().Get<ITestService>().GetValue(), 10);

    gx::ServiceLocator::Instance().Register<ITestService>(svc2);
    EXPECT_EQ(gx::ServiceLocator::Instance().Get<ITestService>().GetValue(), 20);
}

} // namespace
