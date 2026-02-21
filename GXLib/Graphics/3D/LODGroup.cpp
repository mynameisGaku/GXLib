#include "pch.h"
/// @file LODGroup.cpp
/// @brief LOD（Level of Detail）グループ管理の実装

#include "Graphics/3D/LODGroup.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/Model.h"

namespace GX
{

void LODGroup::AddLevel(Model* model, float screenPercentage)
{
    LODLevel level;
    level.model = model;
    level.screenPercentage = screenPercentage;
    m_levels.push_back(level);

    // screenPercentage降順にソート（LOD0=最高品質=大きい閾値が先頭）
    std::sort(m_levels.begin(), m_levels.end(),
        [](const LODLevel& a, const LODLevel& b)
        {
            return a.screenPercentage > b.screenPercentage;
        });
}

Model* LODGroup::SelectLOD(const Camera3D& camera, const Transform3D& transform,
                            float boundingRadius) const
{
    if (m_levels.empty())
        return nullptr;

    // カメラ位置とオブジェクト位置の距離を計算
    const XMFLOAT3& camPos = camera.GetPosition();
    const XMFLOAT3& objPos = transform.GetPosition();

    XMVECTOR vCam = XMLoadFloat3(&camPos);
    XMVECTOR vObj = XMLoadFloat3(&objPos);
    XMVECTOR vDiff = XMVectorSubtract(vObj, vCam);

    float distance = 0.0f;
    XMStoreFloat(&distance, XMVector3Length(vDiff));

    // 距離が非常に小さい場合はLOD0を返す
    if (distance < 0.001f)
    {
        m_lastSelectedLevel = 0;
        return m_levels[0].model;
    }

    // カリング距離チェック
    if (m_cullDistance > 0.0f && distance > m_cullDistance)
    {
        return nullptr;
    }

    // スクリーン占有率を計算
    // screenPct = boundingRadius / (distance * tan(fov/2))
    // これはオブジェクトが画面の垂直方向に対してどれだけの割合を占めるかを示す
    float halfFovTan = tanf(camera.GetFovY() * 0.5f);
    float screenPct = boundingRadius / (distance * halfFovTan);

    // 0〜1にクランプ
    screenPct = (std::min)(screenPct, 1.0f);
    screenPct = (std::max)(screenPct, 0.0f);

    // LODレベル選択（ヒステリシス付き）
    // levelsはscreenPercentage降順（LOD0=最高品質が先頭）
    // screenPctが閾値以上なら、そのレベルを使用
    // 全レベルの閾値を下回った場合は最低品質（末尾）を使用
    int selectedLevel = static_cast<int>(m_levels.size()) - 1;

    for (int i = 0; i < static_cast<int>(m_levels.size()); ++i)
    {
        float threshold = m_levels[i].screenPercentage;

        // ヒステリシス: 現在のレベルから離れる方向には閾値にバンドを適用
        if (i > m_lastSelectedLevel)
        {
            // より低品質なLODに切り替える場合 — 閾値を少し下げて切り替えにくくする
            threshold -= k_Hysteresis;
        }
        else if (i < m_lastSelectedLevel)
        {
            // より高品質なLODに切り替える場合 — 閾値を少し上げて切り替えにくくする
            threshold += k_Hysteresis;
        }

        if (screenPct >= threshold)
        {
            selectedLevel = i;
            break;
        }
    }

    m_lastSelectedLevel = selectedLevel;
    return m_levels[selectedLevel].model;
}

} // namespace GX
