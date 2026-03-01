#pragma once
/// @file QuestSystem.h
/// @brief クエスト管理システム
///
/// クエストの定義・進行状態・目標達成・報酬を管理する。
/// 前提条件（prerequisites）の自動チェック、目標完了時の自動クリアに対応。
/// OnStateChanged / OnProgress コールバックで状態変化通知を受け取れる。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>
#include <unordered_map>

namespace gx
{

/// @brief クエスト状態
enum class QuestState
{
    Inactive,   ///< 未開始
    Active,     ///< 進行中
    Completed,  ///< 完了
    Failed      ///< 失敗
};

/// @brief クエスト目標
struct QuestObjective
{
    std::string id;           ///< 目標ID
    std::string description;  ///< 目標の説明
    int requiredCount = 1;    ///< 必要達成数
    int currentCount  = 0;    ///< 現在の達成数

    /// @brief 目標が達成済みかどうか
    bool IsComplete() const { return currentCount >= requiredCount; }
};

/// @brief クエスト報酬
struct QuestReward
{
    std::string type;    ///< 報酬タイプ（"item", "exp" 等）
    std::string itemId;  ///< アイテムID（type=="item" の場合）
    int amount = 0;      ///< 数量
};

/// @brief クエスト定義（マスターデータ）
struct QuestDef
{
    std::string questId;                      ///< 一意なクエストID
    std::string title;                        ///< タイトル
    std::string description;                  ///< 説明文
    std::vector<QuestObjective> objectives;   ///< 目標リスト
    std::vector<QuestReward> rewards;         ///< 報酬リスト
    std::vector<std::string> prerequisites;   ///< 前提クエストID
    bool autoComplete = false;                ///< 全目標達成時に自動完了するか
};

/// @brief クエスト状態変更イベント
struct QuestStateChangedEvent
{
    std::string questId;   ///< 対象クエストID
    QuestState  oldState;  ///< 変更前の状態
    QuestState  newState;  ///< 変更後の状態
};

/// @brief クエスト進捗イベント
struct QuestProgressEvent
{
    std::string questId;      ///< 対象クエストID
    std::string objectiveId;  ///< 対象目標ID
    int         oldCount;     ///< 変更前のカウント
    int         newCount;     ///< 変更後のカウント
};

/// @brief クエスト管理クラス
class QuestSystem
{
public:
    /// @brief クエスト定義を登録する
    void RegisterQuest(const QuestDef& def);

    /// @brief クエスト定義を取得する（未登録なら nullptr）
    const QuestDef* GetQuestDef(const std::string& questId) const;

    /// @brief 登録済みクエスト定義数を返す
    size_t GetQuestDefCount() const;

    /// @brief クエストを開始する（前提条件チェックあり）
    /// @return 開始に成功した場合 true
    bool StartQuest(const std::string& questId);

    /// @brief クエストを完了にする
    /// @return 完了に成功した場合 true
    bool CompleteQuest(const std::string& questId);

    /// @brief クエストを失敗にする
    /// @return 失敗に成功した場合 true
    bool FailQuest(const std::string& questId);

    /// @brief クエストの状態を返す（未登録なら Inactive）
    QuestState GetQuestState(const std::string& questId) const;

    /// @brief 目標の進捗を更新する
    /// @return 更新に成功した場合 true
    bool UpdateObjective(const std::string& questId, const std::string& objectiveId, int delta = 1);

    /// @brief 目標の現在のカウントを返す
    int GetObjectiveProgress(const std::string& questId, const std::string& objectiveId) const;

    /// @brief 目標が完了しているか
    bool IsObjectiveComplete(const std::string& questId, const std::string& objectiveId) const;

    /// @brief 全目標が完了しているか
    bool AreAllObjectivesComplete(const std::string& questId) const;

    /// @brief 進行中のクエストID一覧を返す
    std::vector<std::string> GetActiveQuests() const;

    /// @brief 完了済みのクエストID一覧を返す
    std::vector<std::string> GetCompletedQuests() const;

    /// @brief 前提条件を満たしているか
    bool ArePrerequisitesMet(const std::string& questId) const;

    /// @brief 全状態をリセットする
    void Reset();

    /// @brief 状態変更コールバック
    std::function<void(const QuestStateChangedEvent&)> OnStateChanged;

    /// @brief 進捗更新コールバック
    std::function<void(const QuestProgressEvent&)> OnProgress;

private:
    /// @brief 状態変更イベントを発火する
    void NotifyStateChanged(const std::string& questId, QuestState oldState, QuestState newState);

    /// @brief 進捗イベントを発火する
    void NotifyProgress(const std::string& questId, const std::string& objectiveId,
                        int oldCount, int newCount);

    /// @brief 進捗データ内の目標を検索する
    QuestObjective* FindObjective(const std::string& questId, const std::string& objectiveId);
    const QuestObjective* FindObjective(const std::string& questId, const std::string& objectiveId) const;

    std::unordered_map<std::string, QuestDef>                   m_defs;     ///< クエスト定義
    std::unordered_map<std::string, QuestState>                 m_states;   ///< クエスト状態
    std::unordered_map<std::string, std::vector<QuestObjective>> m_progress; ///< 目標進捗
};

} // namespace gx
/// @}
