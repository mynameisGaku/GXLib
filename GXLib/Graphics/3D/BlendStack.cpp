/// @file BlendStack.cpp
/// @brief ブレンドスタックの実装
#include "pch.h"
#include "Graphics/3D/BlendStack.h"

namespace GX
{

void BlendStack::SetLayer(uint32_t index, const BlendLayer& layer)
{
    if (index >= k_MaxLayers) return;
    m_layers[index] = layer;
    m_active[index] = true;
}

void BlendStack::RemoveLayer(uint32_t index)
{
    if (index >= k_MaxLayers) return;
    m_active[index] = false;
    m_layers[index] = {};
}

void BlendStack::SetLayerWeight(uint32_t index, float weight)
{
    if (index >= k_MaxLayers) return;
    m_layers[index].weight = weight;
}

void BlendStack::SetLayerClip(uint32_t index, const AnimationClip* clip)
{
    if (index >= k_MaxLayers) return;
    m_layers[index].clip = clip;
}

const BlendLayer* BlendStack::GetLayer(uint32_t index) const
{
    if (index >= k_MaxLayers) return nullptr;
    if (!m_active[index]) return nullptr;
    return &m_layers[index];
}

uint32_t BlendStack::GetActiveLayerCount() const
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < k_MaxLayers; ++i)
    {
        if (m_active[i]) ++count;
    }
    return count;
}

void BlendStack::Update(float deltaTime, uint32_t jointCount,
                         const TransformTRS* bindPose,
                         TransformTRS* outPose)
{
    if (jointCount == 0) return;

    // バインドポーズで初期化
    // 初学者向け: まず「立ち姿」から始めて、そこにレイヤーを順に適用します。
    for (uint32_t j = 0; j < jointCount; ++j)
    {
        outPose[j] = bindPose ? bindPose[j] : IdentityTRS();
    }

    m_tempPose.resize(jointCount);

    // グループの分割サイズ（32グループにマッピング）
    const uint32_t groupDivisor = jointCount / 32 + 1;

    for (uint32_t i = 0; i < k_MaxLayers; ++i)
    {
        if (!m_active[i]) continue;

        BlendLayer& layer = m_layers[i];
        if (!layer.clip) continue;
        if (layer.weight <= 0.0f) continue;

        // 時間を進める
        layer.time += deltaTime * layer.speed;
        float duration = layer.clip->GetDuration();
        if (duration > 0.0f)
        {
            if (layer.loop)
            {
                layer.time = fmodf(layer.time, duration);
                if (layer.time < 0.0f)
                    layer.time += duration;
            }
            else if (layer.time >= duration)
            {
                layer.time = duration;
            }
        }

        // クリップをサンプリング
        layer.clip->SampleTRS(layer.time, jointCount, m_tempPose.data(), bindPose);

        float w = layer.weight;

        for (uint32_t j = 0; j < jointCount; ++j)
        {
            // マスクビットチェック
            uint32_t jointGroup = j / groupDivisor;
            if (jointGroup < 32 && !(layer.maskBits & (1u << jointGroup)))
                continue;

            if (layer.mode == AnimBlendMode::Override)
            {
                // Override: 現在のポーズからレイヤーポーズへ重みで補間
                // 初学者向け: weight=1なら完全に上書き、0.5なら半々になります。
                outPose[j].translation.x += (m_tempPose[j].translation.x - outPose[j].translation.x) * w;
                outPose[j].translation.y += (m_tempPose[j].translation.y - outPose[j].translation.y) * w;
                outPose[j].translation.z += (m_tempPose[j].translation.z - outPose[j].translation.z) * w;

                XMVECTOR qCur = XMLoadFloat4(&outPose[j].rotation);
                XMVECTOR qLayer = XMLoadFloat4(&m_tempPose[j].rotation);
                XMStoreFloat4(&outPose[j].rotation, XMQuaternionSlerp(qCur, qLayer, w));

                outPose[j].scale.x += (m_tempPose[j].scale.x - outPose[j].scale.x) * w;
                outPose[j].scale.y += (m_tempPose[j].scale.y - outPose[j].scale.y) * w;
                outPose[j].scale.z += (m_tempPose[j].scale.z - outPose[j].scale.z) * w;
            }
            else // Additive
            {
                // Additive: バインドポーズとの差分を加算
                // 初学者向け: レイヤーのポーズが「立ち姿」からどれだけ動いたかを追加します。
                TransformTRS base = bindPose ? bindPose[j] : IdentityTRS();

                outPose[j].translation.x += (m_tempPose[j].translation.x - base.translation.x) * w;
                outPose[j].translation.y += (m_tempPose[j].translation.y - base.translation.y) * w;
                outPose[j].translation.z += (m_tempPose[j].translation.z - base.translation.z) * w;

                // 加算回転: delta = inverse(baseQ) * layerQ, result = curQ * slerp(identity, delta, w)
                XMVECTOR baseQ = XMLoadFloat4(&base.rotation);
                XMVECTOR layerQ = XMLoadFloat4(&m_tempPose[j].rotation);
                XMVECTOR deltaQ = XMQuaternionMultiply(XMQuaternionInverse(baseQ), layerQ);
                XMVECTOR identity = XMQuaternionIdentity();
                XMVECTOR weightedDelta = XMQuaternionSlerp(identity, deltaQ, w);
                XMVECTOR curQ = XMLoadFloat4(&outPose[j].rotation);
                XMStoreFloat4(&outPose[j].rotation,
                    XMQuaternionNormalize(XMQuaternionMultiply(curQ, weightedDelta)));

                outPose[j].scale.x += (m_tempPose[j].scale.x - base.scale.x) * w;
                outPose[j].scale.y += (m_tempPose[j].scale.y - base.scale.y) * w;
                outPose[j].scale.z += (m_tempPose[j].scale.z - base.scale.z) * w;
            }
        }
    }
}

} // namespace GX
