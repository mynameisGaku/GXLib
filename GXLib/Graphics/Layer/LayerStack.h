#pragma once
/// @file LayerStack.h
/// @brief レイヤースタック管理
///
/// 複数のRenderLayerをZ-order順に管理し、合成時にソート済みリストを提供する。

#include "pch.h"
#include "Graphics/Layer/RenderLayer.h"

namespace GX
{

/// @brief レイヤースタック管理クラス
class LayerStack
{
public:
    LayerStack() = default;
    ~LayerStack() = default;

    /// レイヤーを作成して追加
    RenderLayer* CreateLayer(ID3D12Device* device, const std::string& name,
                             int32_t zOrder, uint32_t w, uint32_t h);

    /// 名前でレイヤーを取得
    RenderLayer* GetLayer(const std::string& name);

    /// 名前でレイヤーを削除
    bool RemoveLayer(const std::string& name);

    /// Z-order昇順ソート済みレイヤーリストを取得
    const std::vector<RenderLayer*>& GetSortedLayers();

    /// レイヤー数を取得
    uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }

    /// 全レイヤーをリサイズ
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

    /// ソートが必要であることを通知（Z-order変更時に呼ぶ）
    void MarkDirty() { m_needsSort = true; }

private:
    void SortLayers();

    std::vector<std::unique_ptr<RenderLayer>> m_layers;
    std::vector<RenderLayer*> m_sortedLayers;
    bool m_needsSort = true;
};

} // namespace GX
