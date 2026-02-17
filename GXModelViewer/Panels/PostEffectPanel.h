#pragma once
/// @file PostEffectPanel.h
/// @brief ImGui panel for controlling GXLib PostEffectPipeline parameters

namespace GX { class PostEffectPipeline; }

/// @brief ImGui panel that exposes all PostEffectPipeline parameters
class PostEffectPanel
{
public:
    /// Draw the post-effect controls.
    /// @param pipeline The PostEffectPipeline to read/write parameters.
    void Draw(GX::PostEffectPipeline& pipeline);
};
