#pragma once
/// @file BlendTreeEditor.h
/// @brief Blend tree parameter editor panel

namespace GX { class BlendTree; }

/// @brief ImGui panel for editing blend tree parameters and visualizing nodes
class BlendTreeEditor
{
public:
    /// Draw the blend tree editor panel.
    /// @param blendTree The blend tree to edit (may be nullptr).
    void Draw(GX::BlendTree* blendTree);
};
