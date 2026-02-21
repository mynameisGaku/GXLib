#pragma once
/// @file IBLPanel.h
/// @brief IBL（環境照明）設定パネル

namespace GX { class IBL; class Skybox; class Renderer3D; }

/// @brief IBL設定パネル
class IBLPanel
{
public:
    void Draw(GX::IBL& ibl, GX::Skybox& skybox, GX::Renderer3D& renderer);
    void DrawContent(GX::IBL& ibl, GX::Skybox& skybox, GX::Renderer3D& renderer);

private:
    float m_intensity = 1.0f;
};
