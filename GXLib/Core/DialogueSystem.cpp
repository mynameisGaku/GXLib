/// @file DialogueSystem.cpp
/// @brief 台詞システム実装
#include "pch_common.h"
#include "Core/DialogueSystem.h"

namespace gx
{

const gx::Vector<DialogueChoice> DialogueSystem::s_emptyChoices;

void DialogueSystem::RegisterSequence(const DialogueSequence& sequence)
{
    m_sequences[sequence.id] = sequence;
}

bool DialogueSystem::StartSequence(const gx::String& sequenceId)
{
    auto it = m_sequences.find(sequenceId);
    if (it == m_sequences.end())
        return false;

    m_currentSequence = &it->second;
    m_currentLineIndex = 0;
    m_lineTimer = 0.0f;

    if (m_currentSequence->lines.empty())
    {
        // 行がない場合 -- 選択肢またはチェーンを確認
        if (!m_currentSequence->choices.empty())
        {
            m_state = DialogueState::ShowingChoices;
            if (m_callbacks.onChoicesPresented)
                m_callbacks.onChoicesPresented(m_currentSequence->choices);
        }
        else if (!m_currentSequence->nextSequenceId.empty())
        {
            return StartSequence(m_currentSequence->nextSequenceId);
        }
        else
        {
            EndDialogue();
        }
        return true;
    }

    ShowCurrentLine();
    return true;
}

void DialogueSystem::AdvanceLine()
{
    if (!m_currentSequence)
        return;

    if (m_state == DialogueState::ShowingChoices)
        return; // SelectChoiceを使用する必要がある

    m_currentLineIndex++;

    // 全行を超えたか確認
    if (m_currentLineIndex >= static_cast<int>(m_currentSequence->lines.size()))
    {
        // 行の終端 -- 選択肢があれば表示
        if (!m_currentSequence->choices.empty())
        {
            m_state = DialogueState::ShowingChoices;
            if (m_callbacks.onChoicesPresented)
                m_callbacks.onChoicesPresented(m_currentSequence->choices);
        }
        else if (!m_currentSequence->nextSequenceId.empty())
        {
            // 次のシーケンスにチェーン
            gx::String nextId = m_currentSequence->nextSequenceId;
            StartSequence(nextId);
        }
        else
        {
            EndDialogue();
        }
        return;
    }

    ShowCurrentLine();
}

void DialogueSystem::SelectChoice(int index)
{
    if (m_state != DialogueState::ShowingChoices)
        return;

    if (!m_currentSequence)
        return;

    if (index < 0 || index >= static_cast<int>(m_currentSequence->choices.size()))
        return;

    const auto& choice = m_currentSequence->choices[index];

    // 選択肢のコールバックがあれば発火
    if (choice.callback)
        choice.callback();

    // 次のシーケンスに遷移するか終了
    if (!choice.nextSequenceId.empty())
    {
        gx::String nextId = choice.nextSequenceId;
        StartSequence(nextId);
    }
    else
    {
        EndDialogue();
    }
}

void DialogueSystem::Update(float deltaTime)
{
    if (m_state != DialogueState::ShowingLine)
        return;

    if (!m_currentSequence)
        return;

    if (m_currentLineIndex < 0 || m_currentLineIndex >= static_cast<int>(m_currentSequence->lines.size()))
        return;

    const auto& line = m_currentSequence->lines[m_currentLineIndex];

    // displayDuration > 0 の場合のみ自動送り
    if (line.displayDuration > 0.0f)
    {
        m_lineTimer += deltaTime;
        if (m_lineTimer >= line.displayDuration)
        {
            AdvanceLine();
        }
    }
}

const DialogueLine* DialogueSystem::GetCurrentLine() const
{
    if (!m_currentSequence)
        return nullptr;

    if (m_currentLineIndex < 0 || m_currentLineIndex >= static_cast<int>(m_currentSequence->lines.size()))
        return nullptr;

    return &m_currentSequence->lines[m_currentLineIndex];
}

const gx::Vector<DialogueChoice>& DialogueSystem::GetCurrentChoices() const
{
    if (m_state == DialogueState::ShowingChoices && m_currentSequence)
        return m_currentSequence->choices;

    return s_emptyChoices;
}

void DialogueSystem::EndDialogue()
{
    m_state = DialogueState::Inactive;
    m_currentSequence = nullptr;
    m_currentLineIndex = -1;
    m_lineTimer = 0.0f;

    if (m_callbacks.onSequenceEnded)
        m_callbacks.onSequenceEnded();
}

void DialogueSystem::ShowCurrentLine()
{
    if (!m_currentSequence)
        return;

    if (m_currentLineIndex < 0 || m_currentLineIndex >= static_cast<int>(m_currentSequence->lines.size()))
        return;

    const auto& line = m_currentSequence->lines[m_currentLineIndex];
    m_lineTimer = 0.0f;

    // displayDuration > 0 なら自動送り、それ以外は手動入力待ち
    if (line.displayDuration > 0.0f)
    {
        m_state = DialogueState::ShowingLine;
    }
    else
    {
        m_state = DialogueState::WaitingForInput;
    }

    if (m_callbacks.onLineStarted)
        m_callbacks.onLineStarted(line.speaker, line.text);
}

} // namespace gx
