#pragma once
/// @file ParticlePanel.h
/// @brief 3Dパーティクルシステム設定パネル

namespace GX { class ParticleSystem3D; }

/// @brief 3Dパーティクル設定パネル
class ParticlePanel
{
public:
    void Draw(GX::ParticleSystem3D& system);
    void DrawContent(GX::ParticleSystem3D& system);

private:
    int m_selectedEmitter = -1;
};
