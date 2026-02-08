#pragma once
/// @file Skybox.h
/// @brief プロシージャルスカイボックスレンダラー

#include "pch.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// スカイボックスの定数バッファ
struct SkyboxConstants
{
    XMFLOAT4X4 viewProjection;
    XMFLOAT3   topColor;
    float      padding1;
    XMFLOAT3   bottomColor;
    float      padding2;
    XMFLOAT3   sunDirection;
    float      sunIntensity;
};

/// @brief プロシージャルスカイボックス（グラデーション＋太陽）
class Skybox
{
public:
    Skybox() = default;
    ~Skybox() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device);

    /// スカイの色設定
    void SetColors(const XMFLOAT3& topColor, const XMFLOAT3& bottomColor);

    /// 太陽の設定
    void SetSun(const XMFLOAT3& direction, float intensity);

    /// 描画
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
              const XMFLOAT4X4& viewProjection);

private:
    bool CreatePipelineState(ID3D12Device* device);

    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;
    Buffer                       m_vertexBuffer;
    Buffer                       m_indexBuffer;
    DynamicBuffer                m_constantBuffer;

    XMFLOAT3 m_topColor     = { 0.3f, 0.5f, 0.9f };
    XMFLOAT3 m_bottomColor  = { 0.7f, 0.8f, 0.95f };
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };
    float    m_sunIntensity = 5.0f;
};

} // namespace GX
