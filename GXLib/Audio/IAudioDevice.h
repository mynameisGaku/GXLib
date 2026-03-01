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
/// @addtogroup grp_audio
/// @{

/// @brief オーディオデバイスの抽象インターフェース
class IAudioDevice
{
public:
    virtual ~IAudioDevice() = default;

    /// @brief オーディオエンジンを初期化する
    /// @return 成功した場合true
    virtual bool Initialize() = 0;

    /// @brief オーディオエンジンをシャットダウンする
    virtual void Shutdown() = 0;

    /// @brief マスターボリュームを設定する (0.0=無音, 1.0=最大)
    /// @param volume ボリューム値（0.0〜1.0）
    virtual void SetMasterVolume(float volume) = 0;

    /// @brief ネイティブXAudio2エンジンポインタを取得する
    /// @return IXAudio2ポインタ（未初期化時nullptr）
    virtual IXAudio2* GetNativeEngine() const = 0;

    /// @brief マスターボイスポインタを取得する
    /// @return マスタリングボイスポインタ（未初期化時nullptr）
    virtual IXAudio2MasteringVoice* GetMasterVoice() const = 0;

    /// @brief 3Dオーディオが初期化済みか確認する
    /// @return 初期化済みならtrue
    virtual bool Is3DAudioInitialized() const = 0;

    /// @brief 出力チャンネル数を取得する
    /// @return チャンネル数（通常2=ステレオ）
    virtual uint32_t GetOutputChannelCount() const = 0;
};

/// @}
} // namespace gx
