#pragma once
/// @file AudioMixerUI.h
/// @brief オーディオミキサーUI — バスごとの音量・パン・ルーティングを管理する
///
/// チャンネルストリップ（音量・パン・ミュート・ソロ・VUメーター）の状態を保持し、
/// バス間のルーティング接続（センド）も管理する。DAWのミキサーに相当する機能。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace gx
{

/// @brief チャンネルストリップデータ
struct ChannelStripData
{
    std::string busName;      ///< バス名
    float volume = 1.0f;      ///< 音量（0.0=無音、1.0=最大）
    float pan = 0.0f;         ///< パン（-1.0=左、0.0=中央、1.0=右）
    bool muted = false;       ///< ミュート状態
    bool soloed = false;      ///< ソロ状態
    float vuLeft = 0.0f;      ///< 現在のVUレベル（左）
    float vuRight = 0.0f;     ///< 現在のVUレベル（右）
    float peakLeft = 0.0f;    ///< ピークレベル（左）
    float peakRight = 0.0f;   ///< ピークレベル（右）
};

/// @brief バスルーティング接続
struct BusRoutingConnection
{
    uint32_t srcBusIndex = 0;  ///< 送信元バスインデックス
    uint32_t dstBusIndex = 0;  ///< 送信先バスインデックス
    float sendLevel = 1.0f;    ///< センドレベル（0.0～1.0）
};

/// @brief オーディオミキサーUI
class AudioMixerUI
{
public:
    AudioMixerUI() = default;
    ~AudioMixerUI() = default;

    /// @brief ミキサーUIを初期化する
    /// @param busCount バス数（最低1）
    void Initialize(uint32_t busCount);

    /// @brief バス数を取得する
    uint32_t GetBusCount() const { return static_cast<uint32_t>(m_strips.size()); }

    // --- チャンネルストリップ ---
    const ChannelStripData& GetChannelStrip(uint32_t busIndex) const;
    void SetVolume(uint32_t busIndex, float volume);
    void SetPan(uint32_t busIndex, float pan);
    void SetMuted(uint32_t busIndex, bool muted);
    void SetSoloed(uint32_t busIndex, bool soloed);

    // --- VUメーター ---
    /// @brief VUレベルを設定する
    void SetVULevel(uint32_t busIndex, float left, float right);

    /// @brief VUメーターを時間経過で減衰させる
    /// @param dt フレーム経過時間（秒）
    void UpdateVUMeters(float dt);

    // --- ソロ制御 ---
    /// @brief ソロされたバスが存在する場合、非ソロバスは実質ミュートされる
    bool GetEffectiveMuted(uint32_t busIndex) const;

    /// @brief ソロされたバスの数を取得する
    uint32_t GetSoloCount() const;

    // --- ルーティング ---
    void AddRouting(uint32_t srcBusIndex, uint32_t dstBusIndex, float sendLevel = 1.0f);
    void RemoveRouting(uint32_t srcBusIndex, uint32_t dstBusIndex);
    uint32_t GetRoutingCount() const { return static_cast<uint32_t>(m_routings.size()); }

    // --- バス名 ---
    std::string GetBusName(uint32_t busIndex) const;
    void SetBusName(uint32_t busIndex, const std::string& name);

    // --- リセット ---
    void ResetToDefault();

    /// @brief VU減衰レート（dB/秒相当）
    static constexpr float k_VUDecayRate = 12.0f;

private:
    std::vector<ChannelStripData> m_strips;      ///< チャンネルストリップリスト
    std::vector<BusRoutingConnection> m_routings; ///< バス間ルーティング接続リスト
    ChannelStripData m_dummyStrip; ///< 範囲外参照用ダミー

    static std::string DefaultBusName(uint32_t index);
};

} // namespace gx
/// @}
