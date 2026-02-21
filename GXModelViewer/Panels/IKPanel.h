#pragma once
/// @file IKPanel.h
/// @brief IK（インバースキネマティクス）設定パネル
///
/// SceneEntityにはIKメンバーがないため、パネルはスタンドアロンの
/// FootIK/LookAtIK/CCDIKSolverオブジェクトへのポインタを受け取る。

namespace GX { class FootIK; class LookAtIK; }

/// @brief IK設定パネル
class IKPanel
{
public:
    void Draw(GX::FootIK* footIK, GX::LookAtIK* lookAtIK, bool hasSkeleton);
    void DrawContent(GX::FootIK* footIK, GX::LookAtIK* lookAtIK, bool hasSkeleton);

private:
    float m_targetPos[3]    = { 0.0f, 0.0f, 0.0f };
    float m_poleVector[3]   = { 0.0f, 1.0f, 0.0f };
    float m_lookAtTarget[3] = { 0.0f, 1.5f, 3.0f };
};
