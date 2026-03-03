#pragma once
/// @file ResourceNode.h
/// @brief フレームグラフ リソースノード -- 名前付きGPUリソースとその現在の状態を追跡する
///
/// フレームグラフが管理するGPUリソース（テクスチャ・バッファ）1つを表す。
/// リソースの種別・サイズ・フォーマット・現在のバリア状態を保持し、
/// FrameGraph::Compile() がバリア挿入を自動判断する際に参照する。
/// @addtogroup grp_gfx_framegraph/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief フレームグラフ リソースの種別を記述する
enum class FGResourceType : uint32_t
{
    Texture2D,  ///< 2Dテクスチャ
    Buffer,     ///< バッファ
};

/// @brief フレームグラフ内の単一GPUリソースを表すノード
struct ResourceNode
{
    gx::String name;                                        ///< リソース名
    FGResourceType type = FGResourceType::Texture2D;         ///< リソース種別
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;         ///< テクスチャフォーマット
    uint32_t width = 0;                                      ///< テクスチャ幅（ピクセル）
    uint32_t height = 0;                                     ///< テクスチャ高さ（ピクセル）

    ID3D12Resource* resource = nullptr;                      ///< バインドされた物理リソース

    D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;  ///< 現在のリソースステート
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;  ///< フレーム開始時の初期ステート

    uint32_t id = 0;        ///< リソースID
    uint32_t refCount = 0;  ///< このリソースを読み書きするパスの数
};

} // namespace gx
/// @}
