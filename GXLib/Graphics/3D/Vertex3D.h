#pragma once
/// @file Vertex3D.h
/// @brief 3D頂点フォーマット定義

#include "pch.h"

namespace GX
{

/// @brief 基本3D頂点（32B: 位置+法線+UV）
/// DxLibの VERTEX3D 構造体に相当する最小構成
struct Vertex3D
{
    XMFLOAT3 position;  ///< 頂点座標
    XMFLOAT3 normal;    ///< 法線ベクトル
    XMFLOAT2 texcoord;  ///< テクスチャ座標 (UV)
};

/// @brief PBR用3D頂点（48B: 位置+法線+UV+タンジェント）
/// gxfmt::VertexStandard とバイナリ互換。ノーマルマップに必要なタンジェントを含む
struct Vertex3D_PBR
{
    XMFLOAT3 position;  ///< 頂点座標
    XMFLOAT3 normal;    ///< 法線ベクトル
    XMFLOAT2 texcoord;  ///< テクスチャ座標 (UV)
    XMFLOAT4 tangent;   ///< タンジェントベクトル（w = バイタンジェントの符号、±1）
};

/// @brief スキニング対応3D頂点（80B: PBR頂点 + ボーン情報）
/// gxfmt::VertexSkinned とバイナリ互換。1頂点あたり最大4ボーンの影響を受ける
struct Vertex3D_Skinned
{
    XMFLOAT3 position;  ///< 頂点座標
    XMFLOAT3 normal;    ///< 法線ベクトル
    XMFLOAT2 texcoord;  ///< テクスチャ座標 (UV)
    XMFLOAT4 tangent;   ///< タンジェントベクトル
    XMUINT4  joints;    ///< 影響するボーンのインデックス（最大4つ）
    XMFLOAT4 weights;   ///< 各ボーンの影響度ウェイト（合計1.0）
};

/// D3D12入力レイアウト: Vertex3D 用
inline const D3D12_INPUT_ELEMENT_DESC k_Vertex3DLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/// D3D12入力レイアウト: Vertex3D_PBR 用（48B stride）
inline const D3D12_INPUT_ELEMENT_DESC k_Vertex3DPBRLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/// D3D12入力レイアウト: Vertex3D_Skinned 用（80B stride）
inline const D3D12_INPUT_ELEMENT_DESC k_Vertex3DSkinnedLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "JOINTS",   0, DXGI_FORMAT_R32G32B32A32_UINT,   0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/// Toonアウトライン用入力レイアウト: PBR頂点（slot 0） + スムース法線（slot 1）
inline const D3D12_INPUT_ELEMENT_DESC k_Vertex3DPBROutlineLayout[] =
{
    // slot 0: 既存PBR頂点
    { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    // slot 1: スムース法線
    { "SMOOTHNORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/// Toonアウトライン用入力レイアウト: Skinned頂点（slot 0） + スムース法線（slot 1）
inline const D3D12_INPUT_ELEMENT_DESC k_Vertex3DSkinnedOutlineLayout[] =
{
    // slot 0: 既存Skinned頂点
    { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "JOINTS",       0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "WEIGHTS",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    // slot 1: スムース法線
    { "SMOOTHNORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

} // namespace GX
