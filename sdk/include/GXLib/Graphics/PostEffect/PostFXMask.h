#pragma once
/// @file PostFXMask.h
/// @brief ポストエフェクト選択適用マスク
///
/// レイヤー単位や簡易APIからポストエフェクトの適用を個別制御するためのビットマスク。
/// PostEffectPipeline::Resolve() にマスクを渡すことで、特定のエフェクトだけを適用できる。

#include <cstdint>

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

/// @brief ポストエフェクト個別フラグ（ビットマスク）
enum class PostFXFlag : uint32_t
{
    None            = 0,
    SSAO            = 1 << 0,
    ContactShadows  = 1 << 1,
    Reflections     = 1 << 2,   ///< SSR / RT Reflections
    VolumetricLight = 1 << 3,
    VolumetricClouds= 1 << 4,
    Bloom           = 1 << 5,
    DoF             = 1 << 6,
    MotionBlur      = 1 << 7,
    Outline         = 1 << 8,
    TAA             = 1 << 9,
    ColorGrading    = 1 << 10,
    FXAA            = 1 << 11,
    Vignette        = 1 << 12,
    RTGI            = 1 << 13,
    AutoExposure    = 1 << 14,
    LensFlare       = 1 << 15,
    VolumetricFog   = 1 << 16,
    RTSoftShadows   = 1 << 17,
    SSGI            = 1 << 18,
    All             = 0xFFFFFFFF
};

// --- ビット演算オペレータ ---

inline constexpr PostFXFlag operator|(PostFXFlag a, PostFXFlag b) noexcept
{
    return static_cast<PostFXFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr PostFXFlag operator&(PostFXFlag a, PostFXFlag b) noexcept
{
    return static_cast<PostFXFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline constexpr PostFXFlag operator~(PostFXFlag a) noexcept
{
    return static_cast<PostFXFlag>(~static_cast<uint32_t>(a));
}

inline constexpr PostFXFlag& operator|=(PostFXFlag& a, PostFXFlag b) noexcept
{
    a = a | b;
    return a;
}

inline constexpr PostFXFlag& operator&=(PostFXFlag& a, PostFXFlag b) noexcept
{
    a = a & b;
    return a;
}

/// @brief マスクに指定フラグが含まれているか判定する
inline constexpr bool HasFlag(PostFXFlag mask, PostFXFlag flag) noexcept
{
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

/// @}
} // namespace gx
