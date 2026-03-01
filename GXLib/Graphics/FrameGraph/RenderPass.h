#pragma once
/// @file RenderPass.h
/// @brief フレームグラフ レンダーパス -- リソースアクセス宣言と実行コールバック
///
/// レンダーパス1つ（シャドウ描画・Gバッファ描画・ポストエフェクトなど）を表す。
/// reads/writes でリソースアクセスを宣言しておくことで、
/// FrameGraph::Compile() がパス間に必要なバリアを自動生成できる。
/// @addtogroup grp_gfx_framegraph/// @{

#include "pch_graphics.h"
#include <functional>

namespace gx
{

/// @brief レンダーパスのリソースアクセス宣言
struct ResourceAccess
{
    uint32_t resourceId = 0;                                              ///< アクセス対象リソースID
    D3D12_RESOURCE_STATES requiredState = D3D12_RESOURCE_STATE_COMMON;    ///< 必要なリソースステート
};

/// @brief レンダーパスコールバックに渡される実行コンテキスト
struct PassContext
{
    ID3D12GraphicsCommandList* cmdList = nullptr;  ///< コマンドリスト
    uint32_t frameIndex = 0;                       ///< フレームインデックス
};

/// @brief フレームグラフ内のレンダーパス
struct RenderPass
{
    std::string name;      ///< パス名（デバッグ用）
    uint32_t id = 0;       ///< パスID
    bool enabled = true;   ///< 有効フラグ

    std::vector<ResourceAccess> reads;   ///< 読み取りリソースアクセス宣言
    std::vector<ResourceAccess> writes;  ///< 書き込みリソースアクセス宣言

    /// 実行コールバック -- FrameGraph::Execute()中に呼び出される
    std::function<void(const PassContext&)> execute;

    uint32_t sortOrder = 0;  ///< Compile()で計算される実行順序
};

} // namespace gx
/// @}
