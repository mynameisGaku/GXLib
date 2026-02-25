#pragma once
/// @file VolumetricClouds.h
/// @brief Volumetric Clouds (ray-marching cloud layer)
///
/// Fullscreen pixel-shader pass that ray-marches through a
/// horizontal cloud slab defined by bottom/top altitude.
/// FBM noise drives density; Beer-Lambert governs scattering.

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// GPU constant buffer layout (must match VolumetricClouds.hlsl)
struct CloudConstants
{
    XMFLOAT4X4 invViewProjection;  // offset  0
    XMFLOAT3   cameraPosition;    // offset 64
    float      time;               // offset 76
    XMFLOAT3   sunDirection;       // offset 80
    float      cloudBottom;        // offset 92
    XMFLOAT3   sunColor;           // offset 96
    float      cloudTop;           // offset 108
    float      coverage;           // offset 112
    float      densityMul;         // offset 116
    float      windSpeed;          // offset 120
    float      silverLining;       // offset 124
    XMFLOAT3   windDirection;      // offset 128
    int        marchSteps;         // offset 140
    int        lightSteps;         // offset 144
    XMFLOAT2   screenDimensions;   // offset 148
    float      _pad[1];            // pad to 160
};  // 160B < 256B alignment

/// @brief Ray-marched volumetric cloud layer
class VolumetricClouds
{
public:
    VolumetricClouds() = default;
    ~VolumetricClouds() = default;

    /// @brief Initialise resources, shaders, PSO
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief Execute the cloud pass (HDR space)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera,
                 float elapsedTime);

    /// @brief Handle screen resize
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- Enable / Disable ---
    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    // --- Cloud layer altitudes (world space) ---
    void  SetCloudBottom(float v)  { m_cloudBottom = v; }
    float GetCloudBottom() const   { return m_cloudBottom; }
    void  SetCloudTop(float v)     { m_cloudTop = v; }
    float GetCloudTop() const      { return m_cloudTop; }

    // --- Coverage (0-1) ---
    void  SetCoverage(float v)     { m_coverage = v; }
    float GetCoverage() const      { return m_coverage; }

    // --- Density multiplier ---
    void  SetDensity(float v)      { m_density = v; }
    float GetDensity() const       { return m_density; }

    // --- Wind ---
    void  SetWindSpeed(float v)    { m_windSpeed = v; }
    float GetWindSpeed() const     { return m_windSpeed; }
    void  SetWindDirection(const XMFLOAT3& d) { m_windDirection = d; }
    const XMFLOAT3& GetWindDirection() const  { return m_windDirection; }

    // --- Quality ---
    void SetMarchSteps(int n)      { m_marchSteps = n; }
    int  GetMarchSteps() const     { return m_marchSteps; }
    void SetLightSteps(int n)      { m_lightSteps = n; }
    int  GetLightSteps() const     { return m_lightSteps; }

    // --- Silver lining intensity ---
    void  SetSilverLiningIntensity(float v) { m_silverLining = v; }
    float GetSilverLiningIntensity() const  { return m_silverLining; }

    // --- Sun (shared with scene lights) ---
    void  SetSunDirection(const XMFLOAT3& d) { m_sunDirection = d; }
    void  SetSunColor(const XMFLOAT3& c)     { m_sunColor = c; }

private:
    bool CreatePipelines(ID3D12Device* device);
    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);

    bool m_enabled = false;

    // Cloud parameters
    float    m_cloudBottom  = 1500.0f;
    float    m_cloudTop     = 3000.0f;
    float    m_coverage     = 0.5f;
    float    m_density      = 0.05f;
    float    m_windSpeed    = 10.0f;
    XMFLOAT3 m_windDirection = { 1.0f, 0.0f, 0.0f };
    int      m_marchSteps   = 64;
    int      m_lightSteps   = 6;
    float    m_silverLining  = 0.5f;

    // Sun (set externally)
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };
    XMFLOAT3 m_sunColor     = { 1.0f, 0.98f, 0.95f };

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // Pipeline
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // SRV heap (scene + depth, 2 slots x 2 frames = 4)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;
};

} // namespace GX
