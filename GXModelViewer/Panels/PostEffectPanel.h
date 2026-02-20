#pragma once
/// @file PostEffectPanel.h
/// @brief ImGui panel for controlling GXLib PostEffectPipeline parameters

namespace GX { class PostEffectPipeline; }

/// @brief ImGui panel that exposes all PostEffectPipeline parameters
class PostEffectPanel
{
public:
    /// Draw the post-effect controls (standalone window).
    void Draw(GX::PostEffectPipeline& pipeline);

    /// Draw only the content (no Begin/End), for embedding in a tabbed container.
    void DrawContent(GX::PostEffectPipeline& pipeline);
};
