#pragma once
/// @file Engine.h
/// @brief エンジンファサード（ServiceLocator経由のサービスアクセス）
///
/// 全GXLibサブシステムへの統一アクセスポイント。
/// 内部ではServiceLocatorを使用し、テスト時にはモックサービスを注入可能。
///
/// 使用例:
/// @code
///   auto& audio = gx::Engine::Instance().GetAudioDevice();
///   audio.SetMasterVolume(0.5f);
/// @endcode

#include "Core/ServiceLocator.h"

namespace gx {

// Forward declarations
class IAudioDevice;

/// @brief エンジンファサード（シングルトン）
///
/// ServiceLocatorのラッパーとして、型安全なサービスアクセスを提供する。
/// テスト時には Set*() メソッドでモックサービスを注入できる。
class Engine
{
public:
    /// @brief シングルトンインスタンスを取得する
    static Engine& Instance()
    {
        static Engine instance;
        return instance;
    }

    // ===== サービスアクセス =====

    /// @brief オーディオデバイスを取得する
    IAudioDevice& GetAudioDevice()
    {
        return ServiceLocator::Instance().Get<IAudioDevice>();
    }

    /// @brief サービスが利用可能か確認する
    template<typename T>
    bool HasService() const
    {
        return ServiceLocator::Instance().Has<T>();
    }

    // ===== テスト用: モックサービス注入 =====

    /// @brief オーディオデバイスを差し替える（テスト用）
    void SetAudioDevice(std::shared_ptr<IAudioDevice> device)
    {
        ServiceLocator::Instance().Register<IAudioDevice>(std::move(device));
    }

private:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};

} // namespace gx
