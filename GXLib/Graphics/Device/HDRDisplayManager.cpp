/// @file HDRDisplayManager.cpp
/// @brief HDRディスプレイ検出・設定管理の実装
#include "pch_graphics.h"
#include "Graphics/Device/HDRDisplayManager.h"
#include "Core/Logger.h"

namespace gx
{

bool HDRDisplayManager::Initialize(IDXGIFactory6* factory, IDXGIAdapter1* adapter, HWND hwnd)
{
    if (!factory || !adapter)
    {
        GX_LOG_ERROR("HDRDisplayManager: factory or adapter is null");
        return false;
    }

    m_hdrSupported = false;
    m_activeMode = HDRMode::SDR;
    m_maxLuminance = 80.0f;
    m_minLuminance = 0.0f;
    m_maxFullFrameLuminance = 80.0f;
    m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    // アダプタに接続された出力を列挙し、HDR対応を確認する
    ComPtr<IDXGIOutput> output;
    for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        ComPtr<IDXGIOutput6> output6;
        HRESULT hr = output.As(&output6);
        if (FAILED(hr))
        {
            output.Reset();
            continue;
        }

        DXGI_OUTPUT_DESC1 desc1 = {};
        hr = output6->GetDesc1(&desc1);
        if (FAILED(hr))
        {
            output.Reset();
            continue;
        }

        // ウィンドウが存在するモニターを特定するために、モニターの矩形とウィンドウ位置を比較
        // 簡略化のため、HDR対応出力が1つでもあれば対応とみなす
        if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
            m_hdrSupported = true;
            m_maxLuminance = desc1.MaxLuminance;
            m_minLuminance = desc1.MinLuminance;
            m_maxFullFrameLuminance = desc1.MaxFullFrameLuminance;

            GX_LOG_INFO("HDR display detected:");
            GX_LOG_INFO("  Max Luminance:      %.1f nits", m_maxLuminance);
            GX_LOG_INFO("  Min Luminance:      %.4f nits", m_minLuminance);
            GX_LOG_INFO("  Max Full Frame Lum: %.1f nits", m_maxFullFrameLuminance);
            break;
        }

        output.Reset();
    }

    if (!m_hdrSupported)
    {
        GX_LOG_INFO("HDR display not detected -- SDR mode will be used");
    }

    return true;
}

DXGI_FORMAT HDRDisplayManager::GetRecommendedFormat() const
{
    if (!m_hdrSupported)
    {
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    // scRGBモードではR16G16B16A16_FLOAT（リニアガンマ、広い値域）
    // HDR10モードではR10G10B10A2_UNORM（PQカーブ、Rec.2020色域）
    // 一般的にscRGBの方が扱いやすい（リニア値そのまま出力できる）
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

DXGI_COLOR_SPACE_TYPE HDRDisplayManager::GetRecommendedColorSpace() const
{
    if (!m_hdrSupported)
    {
        return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    }

    // scRGB（リニアガンマ、sRGB色域だが値域は無制限）
    return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
}

bool HDRDisplayManager::EnableHDR(IDXGISwapChain4* swapChain)
{
    if (!swapChain)
    {
        GX_LOG_ERROR("HDRDisplayManager::EnableHDR: swapChain is null");
        return false;
    }

    if (!m_hdrSupported)
    {
        GX_LOG_WARN("HDR not supported on this display -- cannot enable");
        return false;
    }

    DXGI_COLOR_SPACE_TYPE targetColorSpace = GetRecommendedColorSpace();

    // SwapChainがこのカラースペースをサポートしているか確認
    UINT colorSpaceSupport = 0;
    HRESULT hr = swapChain->CheckColorSpaceSupport(targetColorSpace, &colorSpaceSupport);
    if (FAILED(hr) || !(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
        GX_LOG_WARN("SwapChain does not support requested color space (0x%X)", targetColorSpace);
        return false;
    }

    hr = swapChain->SetColorSpace1(targetColorSpace);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to set HDR color space (HRESULT: 0x%08X)", hr);
        return false;
    }

    m_colorSpace = targetColorSpace;

    // カラースペースに応じてモードを設定
    if (targetColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
    {
        m_activeMode = HDRMode::HDR10;
        GX_LOG_INFO("HDR mode enabled: HDR10 (ST.2084 PQ)");
    }
    else if (targetColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709)
    {
        m_activeMode = HDRMode::scRGB;
        GX_LOG_INFO("HDR mode enabled: scRGB (Linear)");
    }

    return true;
}

void HDRDisplayManager::DisableHDR(IDXGISwapChain4* swapChain)
{
    if (!swapChain)
        return;

    DXGI_COLOR_SPACE_TYPE sdrColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    swapChain->SetColorSpace1(sdrColorSpace);

    m_colorSpace = sdrColorSpace;
    m_activeMode = HDRMode::SDR;

    GX_LOG_INFO("HDR mode disabled -- reverted to SDR");
}

void HDRDisplayManager::SetHDRMetadata(IDXGISwapChain4* swapChain, float maxCLL, float maxFALL)
{
    if (!swapChain)
        return;

    if (m_activeMode == HDRMode::SDR)
    {
        // SDRモードではメタデータ不要
        return;
    }

    // DXGI_HDR_METADATA_HDR10構造体を設定
    // 輝度値は0.0001 nit単位 (ST.2086準拠)
    DXGI_HDR_METADATA_HDR10 metadata = {};

    // プライマリーカラー座標 (Rec.2020)
    // 値は0.00002単位で記述する (CIE 1931 xy色度)
    metadata.RedPrimary[0]   = 34000;  // 0.680
    metadata.RedPrimary[1]   = 16000;  // 0.320
    metadata.GreenPrimary[0] = 13250;  // 0.265
    metadata.GreenPrimary[1] = 34500;  // 0.690
    metadata.BluePrimary[0]  = 7500;   // 0.150
    metadata.BluePrimary[1]  = 3000;   // 0.060
    metadata.WhitePoint[0]   = 15635;  // 0.3127
    metadata.WhitePoint[1]   = 16450;  // 0.3290

    // 輝度範囲 (0.0001 nit単位)
    metadata.MaxMasteringLuminance = static_cast<UINT>(m_maxLuminance * 10000.0f);
    metadata.MinMasteringLuminance = static_cast<UINT>(m_minLuminance * 10000.0f);

    // コンテンツ輝度 (1 nit単位)
    metadata.MaxContentLightLevel      = static_cast<UINT16>(maxCLL);
    metadata.MaxFrameAverageLightLevel = static_cast<UINT16>(maxFALL);

    HRESULT hr = swapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(metadata), &metadata);
    if (FAILED(hr))
    {
        GX_LOG_WARN("Failed to set HDR metadata (HRESULT: 0x%08X)", hr);
    }
}

} // namespace gx
