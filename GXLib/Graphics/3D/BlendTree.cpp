/// @file BlendTree.cpp
/// @brief ブレンドツリーの実装
#include "pch.h"
#include "Graphics/3D/BlendTree.h"

namespace GX
{

void BlendTree::AddNode(const BlendTreeNode& node)
{
    m_nodes.push_back(node);
}

void BlendTree::ClearNodes()
{
    m_nodes.clear();
}

float BlendTree::GetDuration() const
{
    float maxDuration = 0.0f;
    for (const auto& node : m_nodes)
    {
        if (node.clip)
            maxDuration = (std::max)(maxDuration, node.clip->GetDuration());
    }
    return maxDuration;
}

void BlendTree::BlendPoses(const TransformTRS* a, const TransformTRS* b,
                            float t, uint32_t count, TransformTRS* out)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        out[i].translation.x = a[i].translation.x + (b[i].translation.x - a[i].translation.x) * t;
        out[i].translation.y = a[i].translation.y + (b[i].translation.y - a[i].translation.y) * t;
        out[i].translation.z = a[i].translation.z + (b[i].translation.z - a[i].translation.z) * t;

        XMVECTOR qa = XMLoadFloat4(&a[i].rotation);
        XMVECTOR qb = XMLoadFloat4(&b[i].rotation);
        XMStoreFloat4(&out[i].rotation, XMQuaternionSlerp(qa, qb, t));

        out[i].scale.x = a[i].scale.x + (b[i].scale.x - a[i].scale.x) * t;
        out[i].scale.y = a[i].scale.y + (b[i].scale.y - a[i].scale.y) * t;
        out[i].scale.z = a[i].scale.z + (b[i].scale.z - a[i].scale.z) * t;
    }
}

void BlendTree::Evaluate(float time, uint32_t jointCount,
                          const TransformTRS* bindPose,
                          TransformTRS* outPose) const
{
    if (m_nodes.empty() || jointCount == 0)
    {
        for (uint32_t i = 0; i < jointCount; ++i)
            outPose[i] = bindPose ? bindPose[i] : IdentityTRS();
        return;
    }

    if (m_nodes.size() == 1)
    {
        if (m_nodes[0].clip)
            m_nodes[0].clip->SampleTRS(time, jointCount, outPose, bindPose);
        else
            for (uint32_t i = 0; i < jointCount; ++i)
                outPose[i] = bindPose ? bindPose[i] : IdentityTRS();
        return;
    }

    switch (m_type)
    {
    case BlendTreeType::Simple1D:
        Evaluate1D(time, jointCount, bindPose, outPose);
        break;
    case BlendTreeType::SimpleDirectional2D:
        Evaluate2D(time, jointCount, bindPose, outPose);
        break;
    }
}

void BlendTree::Evaluate1D(float time, uint32_t jointCount,
                             const TransformTRS* bindPose,
                             TransformTRS* outPose) const
{
    // ノードをthreshold昇順でソートし、パラメータ値が収まる区間を探す
    std::vector<uint32_t> sorted(m_nodes.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_nodes.size()); ++i)
        sorted[i] = i;
    std::sort(sorted.begin(), sorted.end(), [this](uint32_t a, uint32_t b)
    {
        return m_nodes[a].threshold < m_nodes[b].threshold;
    });

    // パラメータをクランプ
    float param = m_param1D;
    float minT = m_nodes[sorted.front()].threshold;
    float maxT = m_nodes[sorted.back()].threshold;
    param = (std::max)(minT, (std::min)(maxT, param));

    // 隣接する2ノードを検索
    uint32_t idxA = sorted[0];
    uint32_t idxB = sorted[0];
    float t = 0.0f;

    for (uint32_t i = 0; i + 1 < static_cast<uint32_t>(sorted.size()); ++i)
    {
        uint32_t ia = sorted[i];
        uint32_t ib = sorted[i + 1];
        if (param >= m_nodes[ia].threshold && param <= m_nodes[ib].threshold)
        {
            idxA = ia;
            idxB = ib;
            float range = m_nodes[ib].threshold - m_nodes[ia].threshold;
            t = (range > 0.0001f) ? (param - m_nodes[ia].threshold) / range : 0.0f;
            break;
        }
    }

    // 2つのクリップをサンプリングしてブレンド
    m_tempA.resize(jointCount);
    m_tempB.resize(jointCount);

    if (m_nodes[idxA].clip)
        m_nodes[idxA].clip->SampleTRS(time, jointCount, m_tempA.data(), bindPose);
    else
        for (uint32_t i = 0; i < jointCount; ++i)
            m_tempA[i] = bindPose ? bindPose[i] : IdentityTRS();

    if (m_nodes[idxB].clip)
        m_nodes[idxB].clip->SampleTRS(time, jointCount, m_tempB.data(), bindPose);
    else
        for (uint32_t i = 0; i < jointCount; ++i)
            m_tempB[i] = bindPose ? bindPose[i] : IdentityTRS();

    BlendPoses(m_tempA.data(), m_tempB.data(), t, jointCount, outPose);
}

void BlendTree::Evaluate2D(float time, uint32_t jointCount,
                             const TransformTRS* bindPose,
                             TransformTRS* outPose) const
{
    // 2Dブレンド: パラメータ位置に最も近い3ノードをバリセントリック座標で重み付け合成

    // 各ノードの距離を計算
    struct DistEntry { uint32_t index; float dist; };
    std::vector<DistEntry> entries(m_nodes.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_nodes.size()); ++i)
    {
        float dx = m_nodes[i].position[0] - m_param2D[0];
        float dy = m_nodes[i].position[1] - m_param2D[1];
        entries[i] = { i, dx * dx + dy * dy };
    }

    // 距離でソート
    std::sort(entries.begin(), entries.end(), [](const DistEntry& a, const DistEntry& b)
    {
        return a.dist < b.dist;
    });

    // 最大3ノードを使用
    uint32_t useCount = (std::min)(static_cast<uint32_t>(entries.size()), 3u);

    if (useCount == 1)
    {
        // 1ノードだけ
        if (m_nodes[entries[0].index].clip)
            m_nodes[entries[0].index].clip->SampleTRS(time, jointCount, outPose, bindPose);
        else
            for (uint32_t i = 0; i < jointCount; ++i)
                outPose[i] = bindPose ? bindPose[i] : IdentityTRS();
        return;
    }

    if (useCount == 2)
    {
        // 2ノード: 距離の逆数で重み付け
        m_tempA.resize(jointCount);
        m_tempB.resize(jointCount);

        float d0 = sqrtf((std::max)(entries[0].dist, 0.0001f));
        float d1 = sqrtf((std::max)(entries[1].dist, 0.0001f));
        float invTotal = 1.0f / (1.0f / d0 + 1.0f / d1);
        float w0 = (1.0f / d0) * invTotal;

        if (m_nodes[entries[0].index].clip)
            m_nodes[entries[0].index].clip->SampleTRS(time, jointCount, m_tempA.data(), bindPose);
        else
            for (uint32_t i = 0; i < jointCount; ++i)
                m_tempA[i] = bindPose ? bindPose[i] : IdentityTRS();

        if (m_nodes[entries[1].index].clip)
            m_nodes[entries[1].index].clip->SampleTRS(time, jointCount, m_tempB.data(), bindPose);
        else
            for (uint32_t i = 0; i < jointCount; ++i)
                m_tempB[i] = bindPose ? bindPose[i] : IdentityTRS();

        BlendPoses(m_tempB.data(), m_tempA.data(), w0, jointCount, outPose);
        return;
    }

    // 3ノード: バリセントリック座標で重み付け
    m_tempA.resize(jointCount);
    m_tempB.resize(jointCount);
    m_tempC.resize(jointCount);

    const auto& n0 = m_nodes[entries[0].index];
    const auto& n1 = m_nodes[entries[1].index];
    const auto& n2 = m_nodes[entries[2].index];

    // バリセントリック座標を計算
    float x0 = n0.position[0], y0 = n0.position[1];
    float x1 = n1.position[0], y1 = n1.position[1];
    float x2 = n2.position[0], y2 = n2.position[1];
    float px = m_param2D[0], py = m_param2D[1];

    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    float w0, w1, w2;

    if (fabsf(denom) < 0.0001f)
    {
        // 縮退三角形: 距離の逆数でフォールバック
        float d0 = sqrtf((std::max)(entries[0].dist, 0.0001f));
        float d1 = sqrtf((std::max)(entries[1].dist, 0.0001f));
        float d2 = sqrtf((std::max)(entries[2].dist, 0.0001f));
        float invSum = 1.0f / (1.0f / d0 + 1.0f / d1 + 1.0f / d2);
        w0 = (1.0f / d0) * invSum;
        w1 = (1.0f / d1) * invSum;
        w2 = 1.0f - w0 - w1;
    }
    else
    {
        w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / denom;
        w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / denom;
        w2 = 1.0f - w0 - w1;

        // クランプして正規化（三角形の外の場合）
        w0 = (std::max)(0.0f, w0);
        w1 = (std::max)(0.0f, w1);
        w2 = (std::max)(0.0f, w2);
        float wSum = w0 + w1 + w2;
        if (wSum > 0.0001f)
        {
            w0 /= wSum;
            w1 /= wSum;
            w2 /= wSum;
        }
        else
        {
            w0 = 1.0f; w1 = 0.0f; w2 = 0.0f;
        }
    }

    // 3クリップをサンプリング
    if (n0.clip)
        n0.clip->SampleTRS(time, jointCount, m_tempA.data(), bindPose);
    else
        for (uint32_t i = 0; i < jointCount; ++i)
            m_tempA[i] = bindPose ? bindPose[i] : IdentityTRS();

    if (n1.clip)
        n1.clip->SampleTRS(time, jointCount, m_tempB.data(), bindPose);
    else
        for (uint32_t i = 0; i < jointCount; ++i)
            m_tempB[i] = bindPose ? bindPose[i] : IdentityTRS();

    if (n2.clip)
        n2.clip->SampleTRS(time, jointCount, m_tempC.data(), bindPose);
    else
        for (uint32_t i = 0; i < jointCount; ++i)
            m_tempC[i] = bindPose ? bindPose[i] : IdentityTRS();

    // 3ウェイブレンド: 回転は段階的slerp、位置/スケールは加重平均
    for (uint32_t i = 0; i < jointCount; ++i)
    {
        // Translation: 加重平均
        outPose[i].translation.x = m_tempA[i].translation.x * w0 + m_tempB[i].translation.x * w1 + m_tempC[i].translation.x * w2;
        outPose[i].translation.y = m_tempA[i].translation.y * w0 + m_tempB[i].translation.y * w1 + m_tempC[i].translation.y * w2;
        outPose[i].translation.z = m_tempA[i].translation.z * w0 + m_tempB[i].translation.z * w1 + m_tempC[i].translation.z * w2;

        // Rotation: 段階的slerp (A→B by w1/(w0+w1)), then (AB→C by w2)
        XMVECTOR qA = XMLoadFloat4(&m_tempA[i].rotation);
        XMVECTOR qB = XMLoadFloat4(&m_tempB[i].rotation);
        XMVECTOR qC = XMLoadFloat4(&m_tempC[i].rotation);

        float ab = w0 + w1;
        XMVECTOR qAB;
        if (ab > 0.0001f)
            qAB = XMQuaternionSlerp(qA, qB, w1 / ab);
        else
            qAB = qA;

        XMVECTOR qFinal = XMQuaternionSlerp(qAB, qC, w2);
        XMStoreFloat4(&outPose[i].rotation, XMQuaternionNormalize(qFinal));

        // Scale: 加重平均
        outPose[i].scale.x = m_tempA[i].scale.x * w0 + m_tempB[i].scale.x * w1 + m_tempC[i].scale.x * w2;
        outPose[i].scale.y = m_tempA[i].scale.y * w0 + m_tempB[i].scale.y * w1 + m_tempC[i].scale.y * w2;
        outPose[i].scale.z = m_tempA[i].scale.z * w0 + m_tempB[i].scale.z * w1 + m_tempC[i].scale.z * w2;
    }
}

} // namespace GX
