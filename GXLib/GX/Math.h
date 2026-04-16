#pragma once
/// @file GX/Math.h
/// @brief Math types re-export / 数学型の再エクスポート — Layer 1 facade per ADR-0017
///
/// Convenience header that includes all commonly-used math types:
/// vectors, matrices, quaternions, color, random, splines, and math utilities.
///
/// 数学関連の型 (ベクトル/行列/クォータニオン/色/乱数/スプライン) を一括で
/// インクルードするための便利ヘッダ。ヘッダ自体はラッパーなので、本体API
/// のDoxygenは `Math/` 配下の各ヘッダを参照してください。
///
/// All types live in the `gx::` namespace.
/// すべての型は `gx::` 名前空間に属します。
///
/// @code
/// #include "GX/Math.h"
/// gx::Vector3 pos(1.0f, 2.0f, 3.0f);
/// gx::Quaternion rot = gx::Quaternion::FromEulerAngles(0, 1.57f, 0);
/// gx::Matrix4x4 world = gx::Matrix4x4::Translation(pos) * rot.ToMatrix();
/// gx::Color white(1.0f, 1.0f, 1.0f, 1.0f);
/// float t = gx::MathUtil::Lerp(0.0f, 100.0f, 0.5f);  // = 50
/// gx::Random rng(42);
/// float r = rng.NextFloat();   // [0, 1)
/// @endcode
///
/// @addtogroup grp_gx_facade
/// @{

#include "Math/Vector2.h"    ///< 2D vector
#include "Math/Vector3.h"    ///< 3D vector
#include "Math/Vector4.h"    ///< 4D vector
#include "Math/Matrix4x4.h"  ///< 4x4 matrix (row-major, compatible with DX12)
#include "Math/Quaternion.h" ///< Quaternion (WXYZ)
#include "Math/Color.h"      ///< Color (RGBA, 0..1 float)
#include "Math/MathUtil.h"   ///< Lerp, Clamp, DegToRad, etc.
#include "Math/Random.h"     ///< gx::Random (xorshift, seeded)
#include "Math/Spline.h"     ///< Catmull-Rom, Bezier, Cubic Hermite

/// @}
