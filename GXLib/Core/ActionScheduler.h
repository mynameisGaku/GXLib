#pragma once
/// @file ActionScheduler.h
/// @brief アクションスケジューラ（遅延実行・繰り返し・シーケンス）
///
/// ゲーム内で「N秒後に実行」「M秒おきに繰り返す」「連続アクション」を
/// 簡潔に記述できる。TweenManagerと同じID管理パターン。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief シーケンスの1ステップ（待機 or アクション）
struct SequenceStep
{
    float delay = 0.0f;                    ///< このステップ開始前の待機時間
    std::function<void()> action;          ///< 実行するアクション
};

/// @brief アクションスケジューラ
///
/// RunAfter() で遅延実行、RunEvery() で繰り返し実行、
/// RunSequence() でステップ連続実行を登録できる。
class ActionScheduler
{
public:
    /// @brief N秒後にアクションを実行する
    /// @param delay 遅延時間（秒）
    /// @param action 実行する関数
    /// @return スケジュールID（Cancel用）
    int RunAfter(float delay, std::function<void()> action);

    /// @brief 一定間隔でアクションを繰り返し実行する
    /// @param interval 実行間隔（秒）
    /// @param action 実行する関数
    /// @param repeatCount 繰り返し回数（0=無限）
    /// @return スケジュールID（Cancel用）
    int RunEvery(float interval, std::function<void()> action, int repeatCount = 0);

    /// @brief シーケンス（ステップ連続実行）を登録する
    /// @param steps ステップ配列
    /// @return スケジュールID（Cancel用）
    int RunSequence(std::vector<SequenceStep> steps);

    /// @brief 指定IDのスケジュールをキャンセルする
    /// @param id スケジュールID
    void Cancel(int id);

    /// @brief 全スケジュールをキャンセルする
    void CancelAll();

    /// @brief スケジューラを更新する（毎フレーム呼ぶ）
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

    /// @brief アクティブなスケジュール数を取得する
    /// @return 現在アクティブなスケジュールの数
    int GetActiveCount() const;

    /// @brief 指定IDがアクティブか判定する
    /// @param id 確認するスケジュールID
    /// @return アクティブなら true
    bool IsActive(int id) const;

private:
    /// @brief スケジュールエントリの種別
    enum class EntryType
    {
        Delayed,    ///< 遅延実行（RunAfter）
        Repeating,  ///< 繰り返し実行（RunEvery）
        Sequence    ///< シーケンス実行（RunSequence）
    };

    /// @brief スケジュールエントリ（内部管理用）
    struct Entry
    {
        int       id          = 0;            ///< スケジュールID
        EntryType type        = EntryType::Delayed; ///< エントリ種別
        bool      alive       = true;         ///< 有効フラグ（Cancel時にfalse）
        float     timer       = 0.0f;         ///< 経過タイマー（秒）
        float     interval    = 0.0f;         ///< 実行間隔（秒、Repeating用）
        int       repeatCount = 0;            ///< 目標繰り返し回数（0=無限）
        int       repeatDone  = 0;            ///< 完了した繰り返し回数
        std::function<void()>    action;      ///< 実行するアクション
        std::vector<SequenceStep> steps;      ///< シーケンスのステップ配列
        int       currentStep = 0;            ///< 現在のシーケンスステップ番号
    };

    std::vector<Entry> m_entries;  ///< スケジュールエントリ配列
    int m_nextId = 1;              ///< 次に割り当てるスケジュールID
};

} // namespace gx
/// @}
