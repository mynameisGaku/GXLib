#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Matrix4x4.h"
#include "Color.h"

/// @file MathConvert.h
/// @brief GX Math型とDirectXMath型の明示的変換ヘルパー
///
/// math型ヘッダーからXMFLOAT暗黙変換を除去したため、
/// DirectXMath APIとの連携にはこのヘッダーの変換関数を使用する。

#include <DirectXMath.h>
using namespace DirectX;

namespace gx {

// ============================================================================
// ポインタ変換 (XMLoadFloat3等で使用) — reinterpret_cast ラッパー
// バイナリ互換はstatic_assertで保証済み
// ============================================================================

inline const XMFLOAT2* XM(const Vector2* p)    { return reinterpret_cast<const XMFLOAT2*>(p); }
inline XMFLOAT2*       XM(Vector2* p)          { return reinterpret_cast<XMFLOAT2*>(p); }
inline const XMFLOAT3* XM(const Vector3* p)    { return reinterpret_cast<const XMFLOAT3*>(p); }
inline XMFLOAT3*       XM(Vector3* p)          { return reinterpret_cast<XMFLOAT3*>(p); }
inline const XMFLOAT4* XM(const Vector4* p)    { return reinterpret_cast<const XMFLOAT4*>(p); }
inline XMFLOAT4*       XM(Vector4* p)          { return reinterpret_cast<XMFLOAT4*>(p); }
inline const XMFLOAT4* XM(const Quaternion* p) { return reinterpret_cast<const XMFLOAT4*>(p); }
inline XMFLOAT4*       XM(Quaternion* p)       { return reinterpret_cast<XMFLOAT4*>(p); }
inline const XMFLOAT4X4* XM(const Matrix4x4* p) { return reinterpret_cast<const XMFLOAT4X4*>(p); }
inline XMFLOAT4X4*       XM(Matrix4x4* p)       { return reinterpret_cast<XMFLOAT4X4*>(p); }

// ============================================================================
// 参照変換 (関数引数等で使用)
// ============================================================================

inline const XMFLOAT2& ToXM(const Vector2& v)    { return reinterpret_cast<const XMFLOAT2&>(v); }
inline XMFLOAT2&       ToXM(Vector2& v)          { return reinterpret_cast<XMFLOAT2&>(v); }
inline const XMFLOAT3& ToXM(const Vector3& v)    { return reinterpret_cast<const XMFLOAT3&>(v); }
inline XMFLOAT3&       ToXM(Vector3& v)          { return reinterpret_cast<XMFLOAT3&>(v); }
inline const XMFLOAT4& ToXM(const Vector4& v)    { return reinterpret_cast<const XMFLOAT4&>(v); }
inline XMFLOAT4&       ToXM(Vector4& v)          { return reinterpret_cast<XMFLOAT4&>(v); }
inline const XMFLOAT4& ToXM(const Quaternion& q) { return reinterpret_cast<const XMFLOAT4&>(q); }
inline XMFLOAT4&       ToXM(Quaternion& q)       { return reinterpret_cast<XMFLOAT4&>(q); }
inline const XMFLOAT4X4& ToXM(const Matrix4x4& m) { return reinterpret_cast<const XMFLOAT4X4&>(m); }
inline XMFLOAT4X4&       ToXM(Matrix4x4& m)       { return reinterpret_cast<XMFLOAT4X4&>(m); }

// ============================================================================
// 値変換 (XMFLOAT → GX型)
// ============================================================================

inline Vector2    FromXM(const XMFLOAT2& v) { return { v.x, v.y }; }
inline Vector3    FromXM(const XMFLOAT3& v) { return { v.x, v.y, v.z }; }
inline Vector4    FromXM(const XMFLOAT4& v) { return { v.x, v.y, v.z, v.w }; }
inline Quaternion QuatFromXM(const XMFLOAT4& q) { return { q.x, q.y, q.z, q.w }; }
inline Matrix4x4  FromXM(const XMFLOAT4X4& m)
{
    Matrix4x4 r;
    std::memcpy(&r, &m, 64);
    return r;
}

// ============================================================================
// XMMATRIX変換
// ============================================================================

inline XMMATRIX ToXMMATRIX(const Matrix4x4& m)
{
    return XMLoadFloat4x4(XM(&m));
}

inline Matrix4x4 FromXMMATRIX(XMMATRIX xm)
{
    Matrix4x4 r;
    XMStoreFloat4x4(XM(&r), xm);
    return r;
}

// ============================================================================
// バイナリ互換 static_assert
// ============================================================================

// ============================================================================
// Color → XMFLOAT4
// ============================================================================

inline XMFLOAT4 ToXMFLOAT4(const Color& c) { return { c.r, c.g, c.b, c.a }; }

// ============================================================================
// バイナリ互換 static_assert
// ============================================================================

static_assert(sizeof(Vector2) == sizeof(XMFLOAT2), "Vector2/XMFLOAT2 size mismatch");
static_assert(sizeof(Vector3) == sizeof(XMFLOAT3), "Vector3/XMFLOAT3 size mismatch");
static_assert(sizeof(Vector4) == sizeof(XMFLOAT4), "Vector4/XMFLOAT4 size mismatch");
static_assert(sizeof(Quaternion) == sizeof(XMFLOAT4), "Quaternion/XMFLOAT4 size mismatch");
static_assert(sizeof(Matrix4x4) == sizeof(XMFLOAT4X4), "Matrix4x4/XMFLOAT4X4 size mismatch");

} // namespace gx
