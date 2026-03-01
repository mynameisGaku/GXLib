#pragma once
/// @file AudioOcclusion.h
/// @brief オーディオオクルージョン（音の遮蔽計算）
///
/// リスナーとエミッター間のレイキャストコールバックを用いて
/// 遮蔽度を計算し、音量減衰とローパスフィルタを適用する。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"
#include <DirectXMath.h>

namespace gx
{

/// @brief オクルージョン計算結果
struct OcclusionParams
{
    float directLevel   = 1.0f; ///< 直接音レベル (0..1)
    float reverbLevel   = 1.0f; ///< リバーブ送りレベル (0..1)
    float lowPassCutoff = 1.0f; ///< ローパスフィルタカットオフ (0..1, 1=フル帯域)
};

/// @brief オーディオオクルージョン計算器
class AudioOcclusion
{
public:
    /// @brief リスナー位置を設定する
    /// @param pos リスナーのワールド座標
    void SetListenerPosition(const DirectX::XMFLOAT3& pos) { m_listenerPos = pos; }

    /// @brief レイキャストコールバックを設定する
    /// @param raycast from→to の遮蔽度 (0=遮蔽なし, 1=完全遮蔽) を返す関数
    void SetOcclusionCallback(std::function<float(const DirectX::XMFLOAT3& from,
                                                   const DirectX::XMFLOAT3& to)> raycast)
    {
        m_raycastCallback = std::move(raycast);
    }

    /// @brief エミッター位置に対するオクルージョンパラメータを計算する
    /// @param emitterPos エミッターのワールド座標
    /// @return 遮蔽パラメータ
    OcclusionParams Calculate(const DirectX::XMFLOAT3& emitterPos) const;

    /// @brief オクルージョンの有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief オクルージョンが有効かどうか
    bool IsEnabled() const { return m_enabled; }

private:
    bool m_enabled = true;
    DirectX::XMFLOAT3 m_listenerPos = { 0.0f, 0.0f, 0.0f };
    std::function<float(const DirectX::XMFLOAT3&, const DirectX::XMFLOAT3&)> m_raycastCallback;
};

} // namespace gx
/// @}
