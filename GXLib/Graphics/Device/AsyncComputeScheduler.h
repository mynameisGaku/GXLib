#pragma once
/// @file AsyncComputeScheduler.h
/// @brief 非同期コンピュートスケジューラ
///
/// AsyncComputeQueue を活用し、複数のコンピュートタスクを
/// フレーム単位でスケジュール・管理する上位レイヤー。
/// タスクごとに非同期実行の有効/無効を切り替え可能で、
/// パフォーマンスプロファイリングやデバッグ時の同期実行にも対応する。
/// @addtogroup grp_gfx_device/// @{

#include "pch_graphics.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace gx
{

/// @brief 非同期コンピュートタスク種別
enum class AsyncComputeTask
{
    ParticleSimulation,  ///< パーティクルシミュレーション
    LightCulling,        ///< ライトカリング
    SSAOGenerate,        ///< SSAO生成
    HiZGenerate,         ///< Hi-Zバッファ生成
    Count                ///< タスク種別の総数
};

/// @brief タスク実行情報
struct AsyncTaskInfo
{
    AsyncComputeTask task = AsyncComputeTask::ParticleSimulation;  ///< タスク種別
    bool asyncEnabled = false;                                     ///< 非同期実行が有効か
    bool executing = false;                                        ///< 現在実行中か
    uint64_t fenceValue = 0;                                       ///< 完了待ち用フェンス値
};

/// @brief 非同期コンピュートスケジューラ
class AsyncComputeScheduler
{
public:
    AsyncComputeScheduler() = default;
    ~AsyncComputeScheduler() = default;

    /// @brief 初期化
    /// @param device D3D12デバイスへのポインタ
    /// @return 初期化に成功した場合true
    bool Initialize(ID3D12Device* device);

    /// @brief タスクの非同期有効/無効設定
    /// @param task 対象のタスク種別
    /// @param enabled trueで非同期有効
    void SetTaskAsync(AsyncComputeTask task, bool enabled);

    /// @brief タスクが非同期か確認する
    /// @param task 対象のタスク種別
    /// @return 非同期が有効であればtrue
    bool IsTaskAsync(AsyncComputeTask task) const;

    /// @brief フレーム開始
    void BeginFrame();

    /// @brief フレーム終了（フェンス待機）
    void EndFrame();

    /// @brief タスク実行をスケジュール
    /// @param task スケジュールするタスク種別
    void ScheduleTask(AsyncComputeTask task);

    /// @brief タスクの完了を待機
    /// @param task 待機するタスク種別
    void WaitForTask(AsyncComputeTask task);

    /// @brief スケジュール済みタスク数を取得する
    /// @return 現在フレームでスケジュール済みのタスク数
    uint32_t GetScheduledTaskCount() const { return m_scheduledCount; }

    /// @brief 非同期有効タスク数を取得する
    /// @return 非同期実行が有効なタスクの数
    uint32_t GetAsyncEnabledCount() const;

    /// @brief 現在のフレームインデックスを取得する
    /// @return フレームインデックス
    uint64_t GetFrameIndex() const { return m_frameIndex; }

    /// @brief タスク名を取得する
    /// @param task 対象のタスク種別
    /// @return タスク名の文字列
    static gx::String GetTaskName(AsyncComputeTask task);

    /// @brief 全タスクの非同期を無効化
    void DisableAllAsync();

    /// @brief リセット
    void Reset();

private:
    AsyncTaskInfo m_tasks[static_cast<size_t>(AsyncComputeTask::Count)];  ///< タスクごとの実行情報
    uint32_t m_scheduledCount = 0;  ///< 現在フレームのスケジュール済みタスク数
    uint64_t m_frameIndex = 0;      ///< 現在のフレームインデックス
    bool m_initialized = false;     ///< 初期化済みフラグ
};

} // namespace gx
/// @}
