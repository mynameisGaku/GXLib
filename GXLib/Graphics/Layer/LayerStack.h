#pragma once
/// @file LayerStack.h
/// @brief レイヤースタック管理
///
/// DxLibには直接対応する機能が無い。複数のRenderLayerをZ-order順に管理し、
/// LayerCompositorが合成するためのソート済みリストを提供する。
/// レイヤーの作成・検索・削除・ソートを担当する。

#include "pch.h"
#include "Graphics/Layer/RenderLayer.h"

namespace GX
{

/// @brief 複数のRenderLayerをZ-order順に管理するスタッククラス
///
/// レイヤーの生存期間をunique_ptrで管理し、Z-order変更時は遅延ソートする。
class LayerStack
{
public:
    LayerStack() = default;
    ~LayerStack() = default;

    /// @brief レイヤーを新規作成してスタックに追加する
    /// @param device D3D12デバイス
    /// @param name レイヤーの識別名
    /// @param zOrder 合成順序 (小さいほど奥)
    /// @param w 幅
    /// @param h 高さ
    /// @return 作成されたレイヤーのポインタ (所有権はLayerStackが持つ)
    RenderLayer* CreateLayer(ID3D12Device* device, const std::string& name,
                             int32_t zOrder, uint32_t w, uint32_t h);

    /// @brief 名前でレイヤーを検索する
    /// @return 見つからなければnullptr
    RenderLayer* GetLayer(const std::string& name);

    /// @brief 名前を指定してレイヤーを削除する
    /// @return 削除できたらtrue
    bool RemoveLayer(const std::string& name);

    /// @brief Z-order昇順にソートされたレイヤーリストを取得する
    const std::vector<RenderLayer*>& GetSortedLayers();

    /// @brief 現在のレイヤー数
    uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }

    /// @brief 全レイヤーのRTをリサイズする
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

    /// @brief Z-order変更後に呼んで次回GetSortedLayers時にソートを実行させる
    void MarkDirty() { m_needsSort = true; }

private:
    void SortLayers();

    std::vector<std::unique_ptr<RenderLayer>> m_layers;
    std::vector<RenderLayer*> m_sortedLayers;
    bool m_needsSort = true;
};

} // namespace GX
