/// @file Compat_Math.cpp
/// @brief 簡易API 数学ユーティリティ関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatTypes.h"

// ============================================================================
// VECTOR 演算
// ============================================================================
VECTOR VGet(float x, float y, float z)
{
    return { x, y, z };
}

VECTOR VAdd(VECTOR a, VECTOR b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

VECTOR VSub(VECTOR a, VECTOR b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

VECTOR VScale(VECTOR v, float scale)
{
    return { v.x * scale, v.y * scale, v.z * scale };
}

float VDot(VECTOR a, VECTOR b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

VECTOR VCross(VECTOR a, VECTOR b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

VECTOR VNorm(VECTOR v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-8f) return { 0.0f, 0.0f, 0.0f };
    float inv = 1.0f / len;
    return { v.x * inv, v.y * inv, v.z * inv };
}

float VSize(VECTOR v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// ============================================================================
// MATRIX 演算
// ============================================================================
MATRIX MGetIdent()
{
    MATRIX m = {};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

MATRIX MMult(MATRIX a, MATRIX b)
{
    MATRIX result = {};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                result.m[i][j] += a.m[i][k] * b.m[k][j];
    return result;
}

MATRIX MGetRotX(float angle)
{
    MATRIX m = MGetIdent();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[1][1] =  c;  m.m[1][2] = s;
    m.m[2][1] = -s;  m.m[2][2] = c;
    return m;
}

MATRIX MGetRotY(float angle)
{
    MATRIX m = MGetIdent();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[0][0] = c;  m.m[0][2] = -s;
    m.m[2][0] = s;  m.m[2][2] =  c;
    return m;
}

MATRIX MGetRotZ(float angle)
{
    MATRIX m = MGetIdent();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[0][0] =  c;  m.m[0][1] = s;
    m.m[1][0] = -s;  m.m[1][1] = c;
    return m;
}

MATRIX MGetTranslate(VECTOR v)
{
    MATRIX m = MGetIdent();
    m.m[3][0] = v.x;
    m.m[3][1] = v.y;
    m.m[3][2] = v.z;
    return m;
}
