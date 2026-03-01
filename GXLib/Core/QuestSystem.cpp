/// @file QuestSystem.cpp
/// @brief クエスト管理システム実装

#include "pch_common.h"
#include "Core/QuestSystem.h"

#include <algorithm>

namespace gx
{

// ============================================================================
// クエスト定義
// ============================================================================

void QuestSystem::RegisterQuest(const QuestDef& def)
{
    m_defs[def.questId] = def;
}

const QuestDef* QuestSystem::GetQuestDef(const std::string& questId) const
{
    auto it = m_defs.find(questId);
    if (it == m_defs.end()) return nullptr;
    return &it->second;
}

size_t QuestSystem::GetQuestDefCount() const
{
    return m_defs.size();
}

// ============================================================================
// クエスト状態遷移
// ============================================================================

bool QuestSystem::StartQuest(const std::string& questId)
{
    // 定義が存在するか
    const QuestDef* def = GetQuestDef(questId);
    if (!def) return false;

    // 既にアクティブ/完了/失敗なら開始不可
    QuestState current = GetQuestState(questId);
    if (current != QuestState::Inactive) return false;

    // 前提条件チェック
    if (!ArePrerequisitesMet(questId)) return false;

    // 進捗データを初期化（定義の目標をコピー、currentCount=0にリセット）
    std::vector<QuestObjective> objectives = def->objectives;
    for (auto& obj : objectives)
    {
        obj.currentCount = 0;
    }
    m_progress[questId] = std::move(objectives);

    // 状態をActiveに変更
    QuestState oldState = QuestState::Inactive;
    m_states[questId] = QuestState::Active;
    NotifyStateChanged(questId, oldState, QuestState::Active);

    return true;
}

bool QuestSystem::CompleteQuest(const std::string& questId)
{
    auto it = m_states.find(questId);
    if (it == m_states.end()) return false;
    if (it->second != QuestState::Active) return false;

    QuestState oldState = it->second;
    it->second = QuestState::Completed;
    NotifyStateChanged(questId, oldState, QuestState::Completed);

    return true;
}

bool QuestSystem::FailQuest(const std::string& questId)
{
    auto it = m_states.find(questId);
    if (it == m_states.end()) return false;
    if (it->second != QuestState::Active) return false;

    QuestState oldState = it->second;
    it->second = QuestState::Failed;
    NotifyStateChanged(questId, oldState, QuestState::Failed);

    return true;
}

QuestState QuestSystem::GetQuestState(const std::string& questId) const
{
    auto it = m_states.find(questId);
    if (it == m_states.end()) return QuestState::Inactive;
    return it->second;
}

// ============================================================================
// 目標の進捗
// ============================================================================

bool QuestSystem::UpdateObjective(const std::string& questId,
                                   const std::string& objectiveId,
                                   int delta)
{
    // アクティブなクエストのみ更新可能
    QuestState state = GetQuestState(questId);
    if (state != QuestState::Active) return false;

    QuestObjective* obj = FindObjective(questId, objectiveId);
    if (!obj) return false;

    int oldCount = obj->currentCount;
    obj->currentCount += delta;

    // 下限クランプ
    if (obj->currentCount < 0) obj->currentCount = 0;

    NotifyProgress(questId, objectiveId, oldCount, obj->currentCount);

    // autoComplete チェック
    const QuestDef* def = GetQuestDef(questId);
    if (def && def->autoComplete && AreAllObjectivesComplete(questId))
    {
        CompleteQuest(questId);
    }

    return true;
}

int QuestSystem::GetObjectiveProgress(const std::string& questId,
                                       const std::string& objectiveId) const
{
    const QuestObjective* obj = FindObjective(questId, objectiveId);
    if (!obj) return 0;
    return obj->currentCount;
}

bool QuestSystem::IsObjectiveComplete(const std::string& questId,
                                       const std::string& objectiveId) const
{
    const QuestObjective* obj = FindObjective(questId, objectiveId);
    if (!obj) return false;
    return obj->IsComplete();
}

bool QuestSystem::AreAllObjectivesComplete(const std::string& questId) const
{
    auto it = m_progress.find(questId);
    if (it == m_progress.end()) return false;

    const auto& objectives = it->second;
    if (objectives.empty()) return true; // 目標なし = 完了扱い

    return std::all_of(objectives.begin(), objectives.end(),
        [](const QuestObjective& o) { return o.IsComplete(); });
}

// ============================================================================
// クエスト一覧
// ============================================================================

std::vector<std::string> QuestSystem::GetActiveQuests() const
{
    std::vector<std::string> result;
    for (const auto& [id, state] : m_states)
    {
        if (state == QuestState::Active)
        {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> QuestSystem::GetCompletedQuests() const
{
    std::vector<std::string> result;
    for (const auto& [id, state] : m_states)
    {
        if (state == QuestState::Completed)
        {
            result.push_back(id);
        }
    }
    return result;
}

bool QuestSystem::ArePrerequisitesMet(const std::string& questId) const
{
    const QuestDef* def = GetQuestDef(questId);
    if (!def) return false;

    for (const auto& prereq : def->prerequisites)
    {
        QuestState state = GetQuestState(prereq);
        if (state != QuestState::Completed)
        {
            return false;
        }
    }
    return true;
}

void QuestSystem::Reset()
{
    m_states.clear();
    m_progress.clear();
}

// ============================================================================
// 内部ヘルパー
// ============================================================================

void QuestSystem::NotifyStateChanged(const std::string& questId,
                                      QuestState oldState, QuestState newState)
{
    if (OnStateChanged)
    {
        QuestStateChangedEvent evt;
        evt.questId  = questId;
        evt.oldState = oldState;
        evt.newState = newState;
        OnStateChanged(evt);
    }
}

void QuestSystem::NotifyProgress(const std::string& questId,
                                  const std::string& objectiveId,
                                  int oldCount, int newCount)
{
    if (OnProgress)
    {
        QuestProgressEvent evt;
        evt.questId     = questId;
        evt.objectiveId = objectiveId;
        evt.oldCount    = oldCount;
        evt.newCount    = newCount;
        OnProgress(evt);
    }
}

QuestObjective* QuestSystem::FindObjective(const std::string& questId,
                                            const std::string& objectiveId)
{
    auto it = m_progress.find(questId);
    if (it == m_progress.end()) return nullptr;

    auto& objectives = it->second;
    auto objIt = std::find_if(objectives.begin(), objectives.end(),
        [&](const QuestObjective& o) { return o.id == objectiveId; });
    if (objIt == objectives.end()) return nullptr;
    return &(*objIt);
}

const QuestObjective* QuestSystem::FindObjective(const std::string& questId,
                                                  const std::string& objectiveId) const
{
    auto it = m_progress.find(questId);
    if (it == m_progress.end()) return nullptr;

    const auto& objectives = it->second;
    auto objIt = std::find_if(objectives.begin(), objectives.end(),
        [&](const QuestObjective& o) { return o.id == objectiveId; });
    if (objIt == objectives.end()) return nullptr;
    return &(*objIt);
}

} // namespace gx
