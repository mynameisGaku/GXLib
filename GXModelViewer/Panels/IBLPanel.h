#pragma once
/// @file IBLPanel.h
/// @brief IBL（環境照明）設定パネル

namespace gx { class IBL; class Skybox; class Renderer3D; }

/// @brief IBL設定パネル
class IBLPanel
{
public:
    void Draw(gx::IBL& ibl, gx::Skybox& skybox, gx::Renderer3D& renderer);
    void DrawContent(gx::IBL& ibl, gx::Skybox& skybox, gx::Renderer3D& renderer);

private:
    float m_intensity = 1.0f;
};
