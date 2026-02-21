/// @file TextureBrowser.cpp
/// @brief テクスチャブラウザパネル実装
///
/// TextureManager内の全ハンドルをスキャンし、有効なテクスチャをImageButtonで
/// グリッド表示する。GPU SRVハンドルをImTextureIDとして使用。
/// 選択時は拡大プレビューとフォーマット情報を表示する。

#include "TextureBrowser.h"
#include <imgui.h>
#include "Graphics/Resource/Texture.h"

// Helper: format DXGI_FORMAT as a readable string
static const char* FormatName(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:      return "R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
    case DXGI_FORMAT_B8G8R8A8_UNORM:      return "B8G8R8A8_UNORM";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:   return "R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:   return "R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32_FLOAT:            return "R32_FLOAT";
    case DXGI_FORMAT_BC1_UNORM:            return "BC1_UNORM";
    case DXGI_FORMAT_BC3_UNORM:            return "BC3_UNORM";
    case DXGI_FORMAT_BC5_UNORM:            return "BC5_UNORM";
    case DXGI_FORMAT_BC7_UNORM:            return "BC7_UNORM";
    default:                               return "Unknown";
    }
}

void TextureBrowser::Draw(GX::TextureManager& texManager)
{
    if (!ImGui::Begin("Texture Browser"))
    {
        ImGui::End();
        return;
    }

    // --- Controls ---
    ImGui::SliderFloat("Thumbnail Size", &m_thumbnailSize, 32.0f, 256.0f, "%.0f px");
    ImGui::Separator();

    // --- Texture grid ---
    // Iterate over all possible handles (TextureManager uses contiguous handles)
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float cellSize = m_thumbnailSize + 8.0f; // thumbnail + padding
    int columns = static_cast<int>(panelWidth / cellSize);
    if (columns < 1) columns = 1;

    int displayed = 0;

    for (int handle = 0; handle < static_cast<int>(GX::TextureManager::k_MaxTextures); ++handle)
    {
        GX::Texture* tex = texManager.GetTexture(handle);
        if (!tex)
            continue;

        // Skip region-only entries (they share a texture with another handle)
        if (tex->GetWidth() == 0 || tex->GetHeight() == 0)
            continue;

        if (displayed > 0 && (displayed % columns) != 0)
            ImGui::SameLine();

        ImGui::PushID(handle);

        // Highlight selected
        bool isSelected = (handle == m_selectedHandle);
        if (isSelected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.7f));

        // Use the GPU SRV handle as the ImGui texture ID.
        // ImGui_ImplDX12 expects a D3D12_GPU_DESCRIPTOR_HANDLE cast to ImTextureID.
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = tex->GetSRVGPUHandle();
        ImTextureID texId = static_cast<ImTextureID>(gpuHandle.ptr);

        if (ImGui::ImageButton("##thumb", texId, ImVec2(m_thumbnailSize, m_thumbnailSize)))
        {
            m_selectedHandle = handle;
        }

        if (isSelected)
            ImGui::PopStyleColor();

        // Tooltip on hover
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Handle: %d", handle);
            ImGui::Text("Size: %u x %u", tex->GetWidth(), tex->GetHeight());
            ImGui::Text("Format: %s", FormatName(tex->GetFormat()));
            ImGui::EndTooltip();
        }

        ImGui::PopID();
        ++displayed;
    }

    if (displayed == 0)
    {
        ImGui::TextDisabled("No textures loaded.");
    }

    // --- Selected texture details ---
    if (m_selectedHandle >= 0)
    {
        GX::Texture* sel = texManager.GetTexture(m_selectedHandle);
        if (sel)
        {
            ImGui::Separator();
            ImGui::Text("Selected Texture");
            ImGui::Text("  Handle:  %d", m_selectedHandle);
            ImGui::Text("  Size:    %u x %u", sel->GetWidth(), sel->GetHeight());
            ImGui::Text("  Format:  %s", FormatName(sel->GetFormat()));

            // Show preview at larger size
            float previewSize = 200.0f;
            float aspect = static_cast<float>(sel->GetWidth()) / static_cast<float>(sel->GetHeight());
            float previewW = previewSize;
            float previewH = previewSize / aspect;
            if (previewH > previewSize)
            {
                previewH = previewSize;
                previewW = previewSize * aspect;
            }

            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = sel->GetSRVGPUHandle();
            ImTextureID texId = static_cast<ImTextureID>(gpuHandle.ptr);
            ImGui::Image(texId, ImVec2(previewW, previewH));

            // Release button
            if (ImGui::Button("Release Texture"))
            {
                texManager.ReleaseTexture(m_selectedHandle);
                m_selectedHandle = -1;
            }
        }
        else
        {
            // Texture was released externally
            m_selectedHandle = -1;
        }
    }

    ImGui::End();
}
