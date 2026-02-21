#pragma once
/// @file EditorMetadata.h
/// @brief エディタ固有のメタデータ（GXModelViewer用）
///
/// GX::Entity/Componentシステムには含まれない、エディタ固有の表示設定や
/// 選択状態を保持する。将来のSceneGraph→GX::Scene移行時に使用する。

#include <string>

namespace GXEditor
{

/// @brief エンティティごとのエディタ固有メタデータ
struct EditorMetadata
{
    bool showBones = false;         ///< ボーン可視化ON/OFF
    bool showWireframe = false;     ///< ワイヤフレーム表示ON/OFF
    bool showBoundingBox = false;   ///< バウンディングボックス表示ON/OFF
    int  selectedClipIndex = -1;    ///< タイムラインで選択中のアニメーションクリップ
    bool pendingRemoval = false;    ///< 遅延削除フラグ（GPUフラッシュ後に削除）
};

} // namespace GXEditor
