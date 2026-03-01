#pragma once
/// @file SceneViewport.h
/// @brief 3Dシーンをエディタ上でインタラクティブに表示・操作するビューポート
///
/// Select/Move/Rotate/Scale ツール・Perspective/Top/Front 等の視点モード・
/// グリッド表示・カメラのOrbit/Pan操作を持つ。Unityのシーンビューに相当する。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace gx
{

class Scene;

/// @brief ビューポート操作ツール
enum class ViewportTool : uint32_t
{
    Select = 0, ///< 選択ツール
    Move,       ///< 移動ツール
    Rotate,     ///< 回転ツール
    Scale,      ///< スケールツール
    Rect        ///< 矩形選択ツール
};

/// @brief ビューポート表示モード
enum class ViewportMode : uint32_t
{
    Perspective = 0, ///< 透視投影（自由カメラ）
    Top,             ///< 上面図（Y軸方向）
    Front,           ///< 正面図（Z軸方向）
    Right,           ///< 右面図（X軸方向）
    Orthographic     ///< 平行投影（自由カメラ）
};

/// @brief グリッド表示設定
struct GridConfig
{
    bool  visible        = true;                          ///< グリッド表示の有効/無効
    float cellSize       = 1.0f;                          ///< セルサイズ（ワールド単位）
    int   majorLineEvery = 10;                            ///< メジャーラインの間隔（セル数）
    float color[4]       = { 0.5f, 0.5f, 0.5f, 0.3f };   ///< 通常線の色（RGBA）
    float majorColor[4]  = { 0.7f, 0.7f, 0.7f, 0.5f };   ///< メジャー線の色（RGBA）
};

/// @brief ビューポートカメラ
struct ViewportCamera
{
    float position[3]  = { 0.0f, 5.0f, -10.0f }; ///< カメラ位置（ワールド座標）
    float rotation[3]  = { 0.0f, 0.0f, 0.0f };   ///< カメラ回転オイラー角（度）
    float fov          = 60.0f;                    ///< 垂直視野角（度）
    float nearPlane    = 0.1f;                     ///< ニアクリップ距離
    float farPlane     = 1000.0f;                  ///< ファークリップ距離
    float orthoSize    = 10.0f;                    ///< 平行投影時の表示範囲サイズ
    float moveSpeed    = 10.0f;                    ///< カメラ移動速度（ワールド単位/秒）
    float rotateSpeed  = 0.3f;                     ///< カメラ回転速度（度/ピクセル）
};

/// @brief 選択情報
struct SelectionInfo
{
    std::vector<uint32_t> selectedEntities;        ///< 選択中のエンティティIDリスト
    uint32_t primarySelection = 0;                 ///< プライマリ選択のエンティティID
    float boundsMin[3] = { 0.0f, 0.0f, 0.0f };    ///< 選択範囲のAABB最小座標
    float boundsMax[3] = { 0.0f, 0.0f, 0.0f };    ///< 選択範囲のAABB最大座標
};

/// @brief ドラッグ&ドロップペイロード
struct DragDropPayload
{
    enum class Type : uint32_t { Entity, Asset, Prefab };
    Type type            = Type::Entity;
    std::string name;
    std::string data;
    float position[3]    = { 0.0f, 0.0f, 0.0f };
};

/// @brief シーンビューポートエディタ
///
/// 3Dシーンの表示、エンティティの選択・操作、カメラ制御、
/// グリッド表示、スナッピング、ドラッグ&ドロップをサポートする。
class SceneViewport
{
public:
    SceneViewport() = default;
    ~SceneViewport() = default;

    // --- ツール ---
    void SetActiveTool(ViewportTool tool) { m_tool = tool; }
    ViewportTool GetActiveTool() const { return m_tool; }

    // --- 表示モード ---
    void SetViewMode(ViewportMode mode) { m_mode = mode; }
    ViewportMode GetViewMode() const { return m_mode; }

    // --- カメラ ---
    const ViewportCamera& GetCamera() const { return m_camera; }
    void SetCamera(const ViewportCamera& cam) { m_camera = cam; }

    // --- グリッド ---
    void SetGridConfig(const GridConfig& cfg) { m_grid = cfg; }
    const GridConfig& GetGridConfig() const { return m_grid; }

    // --- シーン ---
    void SetScene(Scene* scene) { m_scene = scene; }
    Scene* GetScene() const { return m_scene; }

    // --- 選択 ---
    void Select(uint32_t entityId);
    void Deselect(uint32_t entityId);
    void ClearSelection();
    void SelectAll();
    SelectionInfo GetSelection() const;
    bool IsSelected(uint32_t entityId) const;
    uint32_t GetSelectedCount() const { return static_cast<uint32_t>(m_selection.selectedEntities.size()); }

    /// @brief 選択中エンティティにカメラをフォーカスする
    void FocusSelection();

    /// @brief 選択中エンティティを削除する
    void DeleteSelected();

    /// @brief 選択中エンティティを複製する
    std::vector<uint32_t> DuplicateSelected();

    // --- ドラッグ&ドロップ ---
    void BeginDragDrop(const DragDropPayload& payload);
    void EndDragDrop();
    bool IsDragging() const { return m_dragging; }

    // --- 座標変換 ---

    /// @brief スクリーン座標 → ワールド座標 (簡易アンプロジェクト)
    void ScreenToWorld(float screenX, float screenY, float& outX, float& outY, float& outZ) const;

    /// @brief ワールド座標 → スクリーン座標 (簡易プロジェクト)
    void WorldToScreen(float worldX, float worldY, float worldZ, float& outX, float& outY) const;

    // --- ビューポートサイズ ---
    void SetViewportSize(uint32_t width, uint32_t height) { m_viewportWidth = width; m_viewportHeight = height; }
    uint32_t GetViewportWidth() const { return m_viewportWidth; }
    uint32_t GetViewportHeight() const { return m_viewportHeight; }

    // --- カメラ操作 ---
    void Orbit(float deltaX, float deltaY);
    void Pan(float deltaX, float deltaY);
    void Zoom(float delta);
    void FrameAll();

    // --- スナッピング ---
    void SetSnapping(bool enabled, float gridSnap = 1.0f) { m_snapping = enabled; m_gridSnap = gridSnap; }
    bool IsSnapping() const { return m_snapping; }
    float GetGridSnap() const { return m_gridSnap; }

    // --- コールバック ---
    std::function<void(uint32_t)> OnEntityDeleted;
    std::function<void(uint32_t, uint32_t)> OnEntityDuplicated; // (oldId, newId)

private:
    ViewportTool   m_tool           = ViewportTool::Select;    ///< 現在のアクティブツール
    ViewportMode   m_mode           = ViewportMode::Perspective; ///< ビューポート表示モード
    ViewportCamera m_camera;                                     ///< ビューポートカメラ設定
    GridConfig     m_grid;                                       ///< グリッド表示設定
    SelectionInfo  m_selection;                                  ///< 選択中エンティティ情報
    Scene*         m_scene          = nullptr;                   ///< 対象シーンへのポインタ
    uint32_t       m_viewportWidth  = 1280;                      ///< ビューポート幅（ピクセル）
    uint32_t       m_viewportHeight = 720;                       ///< ビューポート高さ（ピクセル）
    bool           m_dragging       = false;                     ///< ドラッグ操作中か
    DragDropPayload m_dragPayload;                               ///< 現在のドラッグペイロード
    bool           m_snapping       = false;                     ///< スナッピング有効か
    float          m_gridSnap       = 1.0f;                      ///< グリッドスナップ間隔（ワールド単位）
};

} // namespace gx
/// @}
