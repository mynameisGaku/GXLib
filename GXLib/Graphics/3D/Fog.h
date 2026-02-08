#pragma once
/// @file Fog.h
/// @brief フォグパラメータ（Linear/Exp/Exp2）

#include "pch.h"

namespace GX
{

/// フォグモード
enum class FogMode : uint32_t
{
    None    = 0,
    Linear  = 1,
    Exp     = 2,
    Exp2    = 3,
};

/// フォグ定数（FrameConstantsに含まれる）
struct FogConstants
{
    XMFLOAT3  fogColor   = { 0.6f, 0.65f, 0.7f };
    float     fogStart   = 50.0f;
    float     fogEnd     = 200.0f;
    float     fogDensity = 0.01f;
    uint32_t  fogMode    = 0;   // FogMode
    float     padding    = 0.0f;
};

} // namespace GX
