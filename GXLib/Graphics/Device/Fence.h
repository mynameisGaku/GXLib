#pragma once
/// @file Fence.h
/// @brief GPU同期用フェンスクラス
///
/// 【初学者向け解説】
/// CPUとGPUは非同期で動作します。CPUがGPUに描画コマンドを送っても、
/// GPUがすぐに処理を完了するとは限りません。
///
/// フェンスは「GPUの作業完了を待つ」ための仕組みです。
/// イメージとしては、レストランで注文した料理が「できたよ！」と
/// 知らせてくれるベルのようなものです。
///
/// 使い方：
/// 1. コマンドを送信した後、Signal()でフェンス値を設定
/// 2. WaitForValue()でGPUが値に達するまで待機
/// 3. フレーム終了時にWaitForGPU()で完了を保証

#include "pch.h"

namespace GX
{

/// @brief GPU同期用フェンス
class Fence
{
public:
    Fence() = default;
    ~Fence();

    /// フェンスを作成
    bool Initialize(ID3D12Device* device);

    /// 現在のフェンス値をインクリメントしてCommandQueueにSignalする
    uint64_t Signal(ID3D12CommandQueue* queue);

    /// 指定した値にGPUが達するまでCPUを待機させる
    void WaitForValue(uint64_t value);

    /// 現在のフェンス値まで待機（GPUの処理が全て完了するまで待つ）
    void WaitForGPU(ID3D12CommandQueue* queue);

    /// 現在のフェンス値を取得
    uint64_t GetCurrentValue() const { return m_fenceValue; }

    /// GPU側のフェンス完了値を取得
    uint64_t GetCompletedValue() const { return m_fence->GetCompletedValue(); }

private:
    ComPtr<ID3D12Fence> m_fence;
    HANDLE              m_event = nullptr;
    uint64_t            m_fenceValue = 0;
};

} // namespace GX
