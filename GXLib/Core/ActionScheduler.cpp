/// @file ActionScheduler.cpp
/// @brief アクションスケジューラの実装
#include "pch_common.h"
#include "Core/ActionScheduler.h"

namespace gx
{

int ActionScheduler::RunAfter(float delay, std::function<void()> action)
{
    int id = m_nextId++;
    Entry e;
    e.id = id;
    e.type = EntryType::Delayed;
    e.timer = delay;
    e.action = std::move(action);
    m_entries.push_back(std::move(e));
    return id;
}

int ActionScheduler::RunEvery(float interval, std::function<void()> action, int repeatCount)
{
    int id = m_nextId++;
    Entry e;
    e.id = id;
    e.type = EntryType::Repeating;
    e.timer = interval;
    e.interval = interval;
    e.repeatCount = repeatCount;
    e.action = std::move(action);
    m_entries.push_back(std::move(e));
    return id;
}

int ActionScheduler::RunSequence(std::vector<SequenceStep> steps)
{
    if (steps.empty()) return 0;
    int id = m_nextId++;
    Entry e;
    e.id = id;
    e.type = EntryType::Sequence;
    e.steps = std::move(steps);
    e.currentStep = 0;
    e.timer = e.steps[0].delay;
    m_entries.push_back(std::move(e));
    return id;
}

void ActionScheduler::Cancel(int id)
{
    for (auto& e : m_entries)
    {
        if (e.id == id) { e.alive = false; break; }
    }
}

void ActionScheduler::CancelAll()
{
    m_entries.clear();
}

void ActionScheduler::Update(float deltaTime)
{
    for (auto& e : m_entries)
    {
        if (!e.alive) continue;

        e.timer -= deltaTime;

        switch (e.type)
        {
        case EntryType::Delayed:
            if (e.timer <= 0.0f)
            {
                if (e.action) e.action();
                e.alive = false;
            }
            break;

        case EntryType::Repeating:
            if (e.timer <= 0.0f)
            {
                if (e.action) e.action();
                e.repeatDone++;
                if (e.repeatCount > 0 && e.repeatDone >= e.repeatCount)
                {
                    e.alive = false;
                }
                else
                {
                    e.timer += e.interval;
                }
            }
            break;

        case EntryType::Sequence:
            if (e.timer <= 0.0f)
            {
                auto& step = e.steps[e.currentStep];
                if (step.action) step.action();
                e.currentStep++;
                if (e.currentStep >= static_cast<int>(e.steps.size()))
                {
                    e.alive = false;
                }
                else
                {
                    e.timer = e.steps[e.currentStep].delay;
                }
            }
            break;
        }
    }

    // 完了エントリを除去
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
            [](const Entry& e) { return !e.alive; }),
        m_entries.end());
}

int ActionScheduler::GetActiveCount() const
{
    return static_cast<int>(m_entries.size());
}

bool ActionScheduler::IsActive(int id) const
{
    for (const auto& e : m_entries)
    {
        if (e.id == id && e.alive) return true;
    }
    return false;
}

} // namespace gx
