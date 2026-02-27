#pragma once
/// @file IAudioDevice.h
/// @brief オーディオデバイスのインターフェース
///
/// XAudio2への直接依存を隠蔽し、テスト時にはモック実装を注入可能にする。
/// 実際のXAudio2ポインタが必要な場面では GetNativeEngine() を使用。

#include <cstdint>

struct IXAudio2;
struct IXAudio2MasteringVoice;

namespace gx {

/// @brief オーディオデバイスの抽象インターフェース
class IAudioDevice
{
public:
    virtual ~IAudioDevice() = default;

    /// @brief オーディオエンジンを初期化する
    virtual bool Initialize() = 0;

    /// @brief オーディオエンジンをシャットダウンする
    virtual void Shutdown() = 0;

    /// @brief マスターボリュームを設定する (0.0=無音, 1.0=最大)
    virtual void SetMasterVolume(float volume) = 0;

    /// @brief ネイティブXAudio2エンジンポインタを取得する
    virtual IXAudio2* GetNativeEngine() const = 0;

    /// @brief マスターボイスポインタを取得する
    virtual IXAudio2MasteringVoice* GetMasterVoice() const = 0;

    /// @brief 3Dオーディオが初期化済みか確認する
    virtual bool Is3DAudioInitialized() const = 0;

    /// @brief 出力チャンネル数を取得する
    virtual uint32_t GetOutputChannelCount() const = 0;
};

} // namespace gx
