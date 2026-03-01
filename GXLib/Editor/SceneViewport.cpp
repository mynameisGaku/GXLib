/// @file SceneViewport.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/SceneViewport.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cmath>

namespace gx
{

// ============================================================================
// 選択
// ============================================================================

void SceneViewport::Select(uint32_t entityId)
{
    // 既に選択済みなら何もしない
    auto& sel = m_selection.selectedEntities;
    if (std::find(sel.begin(), sel.end(), entityId) != sel.end())
        return;

    sel.push_back(entityId);
    m_selection.primarySelection = entityId;
}

void SceneViewport::Deselect(uint32_t entityId)
{
    auto& sel = m_selection.selectedEntities;
    sel.erase(std::remove(sel.begin(), sel.end(), entityId), sel.end());

    // プライマリ選択を更新
    if (m_selection.primarySelection == entityId)
    {
        m_selection.primarySelection = sel.empty() ? 0 : sel.back();
    }
}

void SceneViewport::ClearSelection()
{
    m_selection.selectedEntities.clear();
    m_selection.primarySelection = 0;
    m_selection.boundsMin[0] = m_selection.boundsMin[1] = m_selection.boundsMin[2] = 0.0f;
    m_selection.boundsMax[0] = m_selection.boundsMax[1] = m_selection.boundsMax[2] = 0.0f;
}

void SceneViewport::SelectAll()
{
    if (!m_scene) return;

    ClearSelection();
    for (const auto& entity : m_scene->GetEntities())
    {
        m_selection.selectedEntities.push_back(entity->GetID());
    }
    if (!m_selection.selectedEntities.empty())
        m_selection.primarySelection = m_selection.selectedEntities.front();
}

SelectionInfo SceneViewport::GetSelection() const
{
    return m_selection;
}

bool SceneViewport::IsSelected(uint32_t entityId) const
{
    const auto& sel = m_selection.selectedEntities;
    return std::find(sel.begin(), sel.end(), entityId) != sel.end();
}

// ============================================================================
// フォーカス / 削除 / 複製
// ============================================================================

void SceneViewport::FocusSelection()
{
    if (!m_scene || m_selection.selectedEntities.empty())
        return;

    // 選択エンティティの平均位置にカメラを向ける
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    uint32_t count = 0;
    for (uint32_t id : m_selection.selectedEntities)
    {
        Entity* e = m_scene->FindEntityByID(id);
        if (e)
        {
            const auto& pos = e->GetTransform().GetPosition();
            cx += pos.x;
            cy += pos.y;
            cz += pos.z;
            count++;
        }
    }

    if (count > 0)
    {
        cx /= static_cast<float>(count);
        cy /= static_cast<float>(count);
        cz /= static_cast<float>(count);

        // カメラを対象の少し後方に配置
        m_camera.position[0] = cx;
        m_camera.position[1] = cy + 5.0f;
        m_camera.position[2] = cz - 10.0f;
    }
}

void SceneViewport::DeleteSelected()
{
    if (!m_scene) return;

    for (uint32_t id : m_selection.selectedEntities)
    {
        Entity* e = m_scene->FindEntityByID(id);
        if (e)
        {
            m_scene->DestroyEntity(e);
            if (OnEntityDeleted) OnEntityDeleted(id);
        }
    }
    ClearSelection();
}

std::vector<uint32_t> SceneViewport::DuplicateSelected()
{
    std::vector<uint32_t> newIds;
    if (!m_scene) return newIds;

    // 選択リストのコピーを作成 (イテレーション中にm_selectionが変わらないように)
    auto selectedCopy = m_selection.selectedEntities;

    for (uint32_t id : selectedCopy)
    {
        Entity* original = m_scene->FindEntityByID(id);
        if (!original) continue;

        Entity* clone = m_scene->CreateEntity(original->GetName() + "_copy");
        clone->GetTransform().SetPosition(original->GetTransform().GetPosition());
        clone->GetTransform().SetRotation(original->GetTransform().GetRotation());
        clone->GetTransform().SetScale(original->GetTransform().GetScale());
        clone->SetActive(original->IsActive());

        uint32_t newId = clone->GetID();
        newIds.push_back(newId);

        if (OnEntityDuplicated) OnEntityDuplicated(id, newId);
    }

    return newIds;
}

// ============================================================================
// ドラッグ&ドロップ
// ============================================================================

void SceneViewport::BeginDragDrop(const DragDropPayload& payload)
{
    m_dragging    = true;
    m_dragPayload = payload;
}

void SceneViewport::EndDragDrop()
{
    m_dragging = false;
    m_dragPayload = DragDropPayload();
}

// ============================================================================
// 座標変換 (簡易版 — GPU なしでもテスト可能)
// ============================================================================

void SceneViewport::ScreenToWorld(float screenX, float screenY,
                                   float& outX, float& outY, float& outZ) const
{
    // NDC [-1, 1] に変換
    float ndcX = (2.0f * screenX / static_cast<float>(m_viewportWidth)) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / static_cast<float>(m_viewportHeight));

    // 簡易的にカメラ位置 + 方向ベクトルとして変換
    float fovRad = m_camera.fov * 3.14159265f / 180.0f;
    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    float tanHalf = std::tan(fovRad * 0.5f);

    // ワールド距離 10.0 の平面上の座標を計算
    float dist = 10.0f;
    outX = m_camera.position[0] + ndcX * tanHalf * aspect * dist;
    outY = m_camera.position[1] + ndcY * tanHalf * dist;
    outZ = m_camera.position[2] + dist;
}

void SceneViewport::WorldToScreen(float worldX, float worldY, float worldZ,
                                   float& outX, float& outY) const
{
    // 簡易プロジェクション
    float dx = worldX - m_camera.position[0];
    float dy = worldY - m_camera.position[1];
    float dz = worldZ - m_camera.position[2];

    float fovRad = m_camera.fov * 3.14159265f / 180.0f;
    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    float tanHalf = std::tan(fovRad * 0.5f);

    float depth = (std::max)(dz, 0.001f);

    float ndcX = dx / (tanHalf * aspect * depth);
    float ndcY = dy / (tanHalf * depth);

    outX = (ndcX + 1.0f) * 0.5f * static_cast<float>(m_viewportWidth);
    outY = (1.0f - ndcY) * 0.5f * static_cast<float>(m_viewportHeight);
}

// ============================================================================
// カメラ操作
// ============================================================================

void SceneViewport::Orbit(float deltaX, float deltaY)
{
    m_camera.rotation[1] += deltaX * m_camera.rotateSpeed; // yaw
    m_camera.rotation[0] += deltaY * m_camera.rotateSpeed; // pitch

    // ピッチを [-89, 89] にクランプ
    m_camera.rotation[0] = (std::max)(-89.0f, (std::min)(89.0f, m_camera.rotation[0]));
}

void SceneViewport::Pan(float deltaX, float deltaY)
{
    m_camera.position[0] += deltaX * m_camera.moveSpeed * 0.01f;
    m_camera.position[1] += deltaY * m_camera.moveSpeed * 0.01f;
}

void SceneViewport::Zoom(float delta)
{
    if (m_mode == ViewportMode::Orthographic || m_mode == ViewportMode::Top ||
        m_mode == ViewportMode::Front || m_mode == ViewportMode::Right)
    {
        m_camera.orthoSize = (std::max)(0.1f, m_camera.orthoSize - delta);
    }
    else
    {
        // カメラを前進/後退
        m_camera.position[2] += delta * m_camera.moveSpeed * 0.1f;
    }
}

void SceneViewport::FrameAll()
{
    if (!m_scene) return;

    const auto& entities = m_scene->GetEntities();
    if (entities.empty()) return;

    // 全エンティティの平均位置を計算
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    for (const auto& entity : entities)
    {
        const auto& pos = entity->GetTransform().GetPosition();
        cx += pos.x;
        cy += pos.y;
        cz += pos.z;
    }
    float n = static_cast<float>(entities.size());
    cx /= n;
    cy /= n;
    cz /= n;

    m_camera.position[0] = cx;
    m_camera.position[1] = cy + 10.0f;
    m_camera.position[2] = cz - 20.0f;
}

} // namespace gx
