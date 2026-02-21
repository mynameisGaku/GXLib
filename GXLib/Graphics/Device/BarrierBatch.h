#pragma once
/// @file BarrierBatch.h
/// @brief リソースバリアのバッチ発行ユーティリティ
///
/// DX12ではリソースの状態遷移（描画先→テクスチャ読み取り等）にバリアが必要。
/// DxLibでは内部処理だが、DX12では1つずつ発行するとGPUの同期が非効率になる。
/// このクラスで複数バリアを蓄積して1回のAPI呼び出しでまとめて発行する。

#include "pch.h"

namespace GX
{

/// @brief リソースバリアをバッチでまとめて発行するユーティリティ
/// @tparam N 最大同時バリア数（スタック配列で確保、デフォルト16）
///
/// スコープを抜けるとデストラクタで自動フラッシュされる。
/// 容量Nを超えた場合もその場で自動フラッシュして継続する。
template<uint32_t N = 16>
class BarrierBatch
{
public:
    /// @brief コマンドリストを指定して構築する
    /// @param cmdList バリアを発行する先のコマンドリスト
    explicit BarrierBatch(ID3D12GraphicsCommandList* cmdList)
        : m_cmdList(cmdList), m_count(0) {}

    /// @brief リソース状態遷移バリアを追加する
    /// @param resource 遷移対象のリソース
    /// @param before 現在の状態
    /// @param after 遷移先の状態
    /// @param subresource サブリソース番号（デフォルトで全サブリソース）
    void Transition(ID3D12Resource* resource,
                    D3D12_RESOURCE_STATES before,
                    D3D12_RESOURCE_STATES after,
                    UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        if (before == after) return; // 同じ状態なら不要

        if (m_count >= N) Flush(); // バッファが満杯なら先にフラッシュ

        auto& b = m_barriers[m_count++];
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.Transition.pResource   = resource;
        b.Transition.StateBefore = before;
        b.Transition.StateAfter  = after;
        b.Transition.Subresource = subresource;
    }

    /// @brief UAVバリアを追加する（ComputeShader書き込み後の同期に使う）
    /// @param resource 対象リソース（nullptrで全UAVリソース）
    void UAV(ID3D12Resource* resource = nullptr)
    {
        if (m_count >= N) Flush();

        auto& b = m_barriers[m_count++];
        b.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        b.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.UAV.pResource = resource;
    }

    /// @brief 蓄積したバリアを一括でコマンドリストに発行する
    void Flush()
    {
        if (m_count > 0)
        {
            m_cmdList->ResourceBarrier(m_count, m_barriers);
            m_count = 0;
        }
    }

    /// @brief デストラクタで残りを自動フラッシュする
    ~BarrierBatch() { Flush(); }

    /// @brief 現在蓄積中のバリア数を取得する
    /// @return バリア数
    uint32_t Count() const { return m_count; }

private:
    ID3D12GraphicsCommandList* m_cmdList;
    D3D12_RESOURCE_BARRIER     m_barriers[N]{};
    uint32_t                   m_count;
};

} // namespace GX
