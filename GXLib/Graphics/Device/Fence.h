#pragma once
/// @file Fence.h
/// @brief GPU-CPU間の同期制御
///
/// CPUとGPUは非同期で動くため、GPUの処理完了を待つ仕組みが要る。
/// DxLibではScreenFlip()内部で自動的にやっているが、DX12では自分で管理する。
/// Signal()でGPU側に目印を置き、WaitForValue()でその目印に達するまでCPUを止める。

#include "pch.h"

namespace GX
{

/// @brief GPU完了を待つためのフェンス（DxLibでは内部処理）
///
/// フレーム毎にSignal→WaitForValueで同期を取る。
/// WaitForGPU()はSignal+Waitをまとめて行う便利関数。
class Fence
{
public:
    Fence() = default;
    ~Fence();

    /// @brief フェンスを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device);

    /// @brief フェンス値をインクリメントしてGPU側にSignalを発行する
    /// @param queue Signal先のコマンドキュー
    /// @return インクリメント後のフェンス値
    uint64_t Signal(ID3D12CommandQueue* queue);

    /// @brief 指定したフェンス値にGPUが達するまでCPUを停止する
    /// @param value 待機対象のフェンス値（Signal()の戻り値）
    void WaitForValue(uint64_t value);

    /// @brief GPUの処理が全て完了するまで待つ（Signal + WaitForValue）
    /// @param queue 対象のコマンドキュー
    void WaitForGPU(ID3D12CommandQueue* queue);

    /// @brief CPU側が発行した最新のフェンス値を取得する
    /// @return 現在のフェンス値
    uint64_t GetCurrentValue() const { return m_fenceValue; }

    /// @brief GPU側が到達済みのフェンス値を取得する
    /// @return GPU完了済みの値（GetCurrentValue未満なら処理途中）
    uint64_t GetCompletedValue() const { return m_fence->GetCompletedValue(); }

private:
    ComPtr<ID3D12Fence> m_fence;
    HANDLE              m_event = nullptr;      ///< GPU完了通知用のWindowsイベント
    uint64_t            m_fenceValue = 0;        ///< 単調増加するフェンスカウンタ
};

} // namespace GX
