#pragma once
/// @file ReverbZone.h
/// @brief 空間リバーブゾーン管理
///
/// リスナー位置に基づいてリバーブパラメータを自動的にブレンドする。
/// 各ゾーンは内径(innerRadius)と外径(outerRadius)を持ち、
/// リスナーが内径内にいれば100%、外径の外なら0%のブレンドを適用する。
/// AudioBus/AudioMixer/AudioEffectには依存せず、ブレンド係数の計算のみ行う。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"

namespace gx
{

/// @brief リバーブゾーンの設定
struct ReverbZoneConfig
{
    float position[3] = {0.0f, 0.0f, 0.0f}; ///< ゾーン中心座標
    float innerRadius = 5.0f;                 ///< 内径（この範囲内はblend=1.0）
    float outerRadius = 15.0f;                ///< 外径（この範囲外はblend=0.0）
    float decayTime = 1.5f;                   ///< リバーブ減衰時間
    float density = 0.8f;                     ///< リバーブ密度
    float diffusion = 0.9f;                   ///< リバーブ拡散
    float wetDryMix = 1.0f;                   ///< ウェット/ドライ比
    bool enabled = true;                      ///< ゾーンの有効/無効
    int priority = 0;                         ///< 優先度（同ブレンド時に高い方を採用）
    std::string name;                         ///< ゾーン名（デバッグ用）
};

/// @brief リバーブゾーンマネージャ
///
/// 複数のリバーブゾーンを管理し、リスナー位置に基づいて
/// アクティブなゾーンとブレンド係数を計算する。
class ReverbZoneManager
{
public:
    ReverbZoneManager() = default;
    ~ReverbZoneManager() = default;

    /// @brief ゾーンを追加する
    /// @param config ゾーン設定
    /// @return ゾーンID（0は無効値）
    uint32_t AddZone(const ReverbZoneConfig& config);

    /// @brief ゾーンを削除する
    /// @param zoneId 削除対象のゾーンID
    void RemoveZone(uint32_t zoneId);

    /// @brief ゾーン設定を更新する
    /// @param zoneId 対象ゾーンID
    /// @param config 新しい設定
    void UpdateZone(uint32_t zoneId, const ReverbZoneConfig& config);

    /// @brief ゾーン設定を取得する
    /// @param zoneId 対象ゾーンID
    /// @return 設定へのポインタ（見つからなければnullptr）
    const ReverbZoneConfig* GetZone(uint32_t zoneId) const;

    /// @brief リスナー位置を指定してゾーン状態を更新する
    /// @param listenerX リスナーX座標
    /// @param listenerY リスナーY座標
    /// @param listenerZ リスナーZ座標
    void Update(float listenerX, float listenerY, float listenerZ);

    /// @brief 現在アクティブなゾーンIDを取得する
    /// @return アクティブゾーンID（なければ0）
    uint32_t GetActiveZoneId() const;

    /// @brief 現在のブレンド係数を取得する
    /// @return ブレンド係数（0.0〜1.0）
    float GetCurrentBlendFactor() const;

    /// @brief 同時アクティブゾーン上限を設定する
    /// @param max 最大数
    void SetMaxActiveZones(int max);

    /// @brief 同時アクティブゾーン上限を取得する
    /// @return 最大数
    int GetMaxActiveZones() const;

    /// @brief 登録済みゾーン数を取得する
    /// @return ゾーン数
    size_t GetZoneCount() const;

    /// @brief マネージャの有効/無効を設定する
    /// @param enabled 有効にするならtrue
    void SetEnabled(bool enabled);

    /// @brief マネージャが有効かどうか
    /// @return 有効ならtrue
    bool IsEnabled() const;

private:
    /// @brief 内部ゾーン状態
    struct ZoneState
    {
        ReverbZoneConfig config;
        uint32_t id = 0;
        float currentBlend = 0.0f;
    };

    std::vector<ZoneState> m_zones;
    uint32_t m_nextId = 1;
    uint32_t m_activeZoneId = 0;
    float m_currentBlend = 0.0f;
    bool m_enabled = true;
    int m_maxActive = 2;
};

} // namespace gx
/// @}
