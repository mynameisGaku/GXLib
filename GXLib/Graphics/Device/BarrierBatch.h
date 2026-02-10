#pragma once
/// @file BarrierBatch.h
/// @brief リソースバリアのバッチ発行ユーティリティ
///
/// 複数のリソースバリアを蓄積し、1回の ResourceBarrier() 呼び出しでまとめて発行する。
/// D3D12 では複数バリアを1回の API 呼び出しで送ると GPU のパイプライン同期が効率的になる。

#include "pch.h"

namespace GX
{

/// @brief リソースバリアをバッチで発行するユーティリティ
/// @tparam N 最大同時バリア数（スタック配列サイズ）
template<uint32_t N = 16>
class BarrierBatch
{
public:
    /// @brief コマンドリストを指定して初期化
    /// @param cmdList バリアを発行するコマンドリスト
    explicit BarrierBatch(ID3D12GraphicsCommandList* cmdList)
        : m_cmdList(cmdList), m_count(0) {}

    /// @brief トランジションバリアを追加する
    /// @param resource 遷移対象のリソース
    /// @param before 現在のステート
    /// @param after 遷移先のステート
    /// @param subresource サブリソースインデックス（デフォルト: 全サブリソース）
    void Transition(ID3D12Resource* resource,
                    D3D12_RESOURCE_STATES before,
                    D3D12_RESOURCE_STATES after,
                    UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        if (before == after) return;

        if (m_count >= N) Flush();

        auto& b = m_barriers[m_count++];
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.Transition.pResource   = resource;
        b.Transition.StateBefore = before;
        b.Transition.StateAfter  = after;
        b.Transition.Subresource = subresource;
    }

    /// @brief UAVバリアを追加する
    /// @param resource UAVリソース（nullptrで全UAV）
    void UAV(ID3D12Resource* resource = nullptr)
    {
        if (m_count >= N) Flush();

        auto& b = m_barriers[m_count++];
        b.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        b.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.UAV.pResource = resource;
    }

    /// @brief 蓄積したバリアを一括発行する
    void Flush()
    {
        if (m_count > 0)
        {
            m_cmdList->ResourceBarrier(m_count, m_barriers);
            m_count = 0;
        }
    }

    /// @brief デストラクタで自動フラッシュ
    ~BarrierBatch() { Flush(); }

    /// @brief 蓄積中のバリア数を取得
    /// @return 現在のバリア数
    uint32_t Count() const { return m_count; }

private:
    ID3D12GraphicsCommandList* m_cmdList;
    D3D12_RESOURCE_BARRIER     m_barriers[N];
    uint32_t                   m_count;
};

} // namespace GX
