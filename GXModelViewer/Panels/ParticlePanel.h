#pragma once
/// @file ParticlePanel.h
/// @brief 3Dパーティクルシステム設定パネル

namespace gx { class ParticleSystem3D; }

/// @brief 3Dパーティクル設定パネル
class ParticlePanel
{
public:
    void Draw(gx::ParticleSystem3D& system);
    void DrawContent(gx::ParticleSystem3D& system);

private:
    int m_selectedEmitter = -1;
};
