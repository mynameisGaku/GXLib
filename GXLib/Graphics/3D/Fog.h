#pragma once
/// @file Fog.h
/// @brief フォグパラメータ（Linear/Exp/Exp2）

#include "pch.h"

namespace GX
{

/// @brief フォグモード（DxLibの SetFogEnable / SetFogMode に相当）
enum class FogMode : uint32_t
{
    None    = 0,  ///< フォグ無効
    Linear  = 1,  ///< 線形フォグ（start〜endで直線的に濃くなる）
    Exp     = 2,  ///< 指数フォグ（距離に応じて指数関数で濃くなる）
    Exp2    = 3,  ///< 指数二乗フォグ（Expより急激に濃くなる）
};

/// @brief フォグ定数（FrameConstants 内に埋め込まれてGPUに送られる）
struct FogConstants
{
    XMFLOAT3  fogColor   = { 0.6f, 0.65f, 0.7f };  ///< フォグの色（DxLibの SetFogColor に相当）
    float     fogStart   = 50.0f;   ///< Linearモードの開始距離（DxLibの SetFogStartEnd に相当）
    float     fogEnd     = 200.0f;  ///< Linearモードの終了距離
    float     fogDensity = 0.01f;   ///< Exp/Exp2モードの密度係数
    uint32_t  fogMode    = 0;       ///< FogMode 列挙値
    float     padding    = 0.0f;    ///< 16バイト境界パディング
};

} // namespace GX
