#pragma once
/// @file IKPanel.h
/// @brief IK（インバースキネマティクス）設定パネル
///
/// SceneEntityにはIKメンバーがないため、パネルはスタンドアロンの
/// FootIK/LookAtIK/CCDIKSolverオブジェクトへのポインタを受け取る。

namespace gx { class FootIK; class LookAtIK; }

/// @brief IK設定パネル
class IKPanel
{
public:
    void Draw(gx::FootIK* footIK, gx::LookAtIK* lookAtIK, bool hasSkeleton);
    void DrawContent(gx::FootIK* footIK, gx::LookAtIK* lookAtIK, bool hasSkeleton);

private:
    float m_targetPos[3]    = { 0.0f, 0.0f, 0.0f }; ///< IKターゲット位置 (x, y, z)
    float m_poleVector[3]   = { 0.0f, 1.0f, 0.0f }; ///< ポールベクトル（膝/肘の向き制御）
    float m_lookAtTarget[3] = { 0.0f, 1.5f, 3.0f }; ///< LookAt IKの注視点 (x, y, z)
};
