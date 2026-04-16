#pragma once
/// @file HDRDisplayManager.h
/// @brief HDRディスプレイ検出・設定管理
///
/// モニターのHDR対応状況を検出し、SwapChainにHDR10またはscRGBカラースペースを設定する。
/// HDR10はST.2084 PQカーブ、scRGBはリニアガンマの広色域出力を意味する。
/// HDR非対応モニターでは自動的にSDRモードにフォールバックする。
/// @addtogroup grp_gfx_device/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief HDR出力モード
enum class HDRMode : uint32_t
{
    SDR = 0,   ///< 標準ダイナミックレンジ (sRGB)
    HDR10,     ///< ST.2084 PQ, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
    scRGB,     ///< リニア scRGB, DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
};

/// @brief HDRディスプレイの検出と設定を管理するクラス
///
/// モニターのHDR能力（最大輝度、色域など）を検出し、
/// SwapChainに適切なカラースペースとメタデータを設定する。
class HDRDisplayManager
{
public:
    /// @brief HDRディスプレイの検出を行う
    /// @param factory DXGIファクトリ
    /// @param adapter 使用中のGPUアダプタ
    /// @param hwnd 対象ウィンドウハンドル
    /// @return 初期化成功でtrue（HDR非対応でもtrueを返す）
    bool Initialize(IDXGIFactory6* factory, IDXGIAdapter1* adapter, HWND hwnd);

    /// @brief HDR出力がサポートされているか
    /// @return モニターがHDR対応ならtrue
    bool IsHDRSupported() const { return m_hdrSupported; }

    /// @brief 現在のモニター能力に応じた推奨バッファフォーマットを返す
    /// @return R16G16B16A16_FLOAT (scRGB) または R10G10B10A2_UNORM (HDR10)
    DXGI_FORMAT GetRecommendedFormat() const;

    /// @brief 推奨カラースペースを返す
    /// @return DXGI_COLOR_SPACE_TYPE
    DXGI_COLOR_SPACE_TYPE GetRecommendedColorSpace() const;

    /// @brief SwapChainにHDRカラースペースを設定する
    /// @param swapChain 対象のSwapChain
    /// @return 成功でtrue
    bool EnableHDR(IDXGISwapChain4* swapChain);

    /// @brief SwapChainをSDRモードに戻す
    /// @param swapChain 対象のSwapChain
    void DisableHDR(IDXGISwapChain4* swapChain);

    /// @brief HDRメタデータ（最大輝度など）をSwapChainに設定する
    /// @param swapChain 対象のSwapChain
    /// @param maxCLL コンテンツ最大輝度 (nits)
    /// @param maxFALL フレーム平均最大輝度 (nits)
    void SetHDRMetadata(IDXGISwapChain4* swapChain, float maxCLL, float maxFALL);

    /// @brief 現在のHDRモードを取得する
    HDRMode GetActiveMode() const { return m_activeMode; }

    /// @brief モニターの最大輝度を取得する (nits)
    float GetMaxLuminance() const { return m_maxLuminance; }

    /// @brief モニターの最小輝度を取得する (nits)
    float GetMinLuminance() const { return m_minLuminance; }

private:
    bool m_hdrSupported = false;                ///< HDR対応フラグ
    HDRMode m_activeMode = HDRMode::SDR;        ///< 現在のモード
    float m_maxLuminance = 80.0f;               ///< ディスプレイ最大輝度 (nits)
    float m_minLuminance = 0.0f;                ///< ディスプレイ最小輝度 (nits)
    float m_maxFullFrameLuminance = 80.0f;      ///< 全画面最大輝度 (nits)
    DXGI_COLOR_SPACE_TYPE m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709; ///< SDR sRGB
};

} // namespace gx
/// @}
