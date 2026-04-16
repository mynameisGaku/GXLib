/// @file SoundBank.cpp
/// @brief キューベースの再生を持つサウンドバンクの実装
#include "pch_audio.h"
#include "Audio/SoundBank.h"
#include "Audio/AudioManager.h"
#include <algorithm>
#include <random>
#include <numeric>

namespace gx
{

int SoundBank::CreateCue(const gx::String& name, SoundCueMode mode)
{
    SoundCue cue;
    cue.name = name;
    cue.mode = mode;
    m_cues.push_back(std::move(cue));
    return static_cast<int>(m_cues.size()) - 1;
}

void SoundBank::AddEntry(int cueIndex, const SoundCueEntry& entry)
{
    if (cueIndex < 0 || cueIndex >= static_cast<int>(m_cues.size()))
        return;

    m_cues[cueIndex].entries.push_back(entry);

    // シャッフルモードの場合、シャッフル順を再初期化
    if (m_cues[cueIndex].mode == SoundCueMode::Shuffle)
    {
        InitializeShuffle(m_cues[cueIndex]);
    }
}

void SoundBank::SetCooldown(int cueIndex, float seconds)
{
    if (cueIndex < 0 || cueIndex >= static_cast<int>(m_cues.size()))
        return;

    m_cues[cueIndex].cooldown = seconds;
}

void SoundBank::SetMaxConcurrent(int cueIndex, int max)
{
    if (cueIndex < 0 || cueIndex >= static_cast<int>(m_cues.size()))
        return;

    m_cues[cueIndex].maxConcurrent = max;
}

int SoundBank::Play(int cueIndex, AudioManager& audioManager, float currentTime)
{
    if (cueIndex < 0 || cueIndex >= static_cast<int>(m_cues.size()))
        return -1;

    auto& cue = m_cues[cueIndex];

    if (cue.entries.empty())
        return -1;

    // クールダウンチェック
    if (cue.cooldown > 0.0f && (currentTime - cue.lastPlayTime) < cue.cooldown)
        return -1;

    // 最大同時再生数チェック
    if (cue.currentConcurrent >= cue.maxConcurrent)
        return -1;

    // モードに基づいてエントリを選択
    int entryIdx = SelectEntry(cue);
    if (entryIdx < 0 || entryIdx >= static_cast<int>(cue.entries.size()))
        return -1;

    const auto& entry = cue.entries[entryIdx];

    // 範囲内でランダムな音量を計算
    float volume = RandomFloat(entry.volumeMin, entry.volumeMax);

    // AudioManagerで再生
    audioManager.PlaySound(entry.handle, volume);

    // ランタイム状態を更新
    cue.lastPlayTime = currentTime;
    cue.currentConcurrent++;

    return entry.handle;
}

int SoundBank::Play3D(int cueIndex, AudioManager& audioManager, float currentTime,
                      float volume)
{
    if (cueIndex < 0 || cueIndex >= static_cast<int>(m_cues.size()))
        return -1;

    auto& cue = m_cues[cueIndex];

    if (cue.entries.empty())
        return -1;

    // クールダウンチェック
    if (cue.cooldown > 0.0f && (currentTime - cue.lastPlayTime) < cue.cooldown)
        return -1;

    // 最大同時再生数チェック
    if (cue.currentConcurrent >= cue.maxConcurrent)
        return -1;

    // モードに基づいてエントリを選択
    int entryIdx = SelectEntry(cue);
    if (entryIdx < 0 || entryIdx >= static_cast<int>(cue.entries.size()))
        return -1;

    const auto& entry = cue.entries[entryIdx];

    // 範囲内でランダムな音量を計算し、指定音量でスケール
    float randomVol = RandomFloat(entry.volumeMin, entry.volumeMax) * volume;

    // AudioManagerで再生（簡易版: 3Dエミッターなし）
    audioManager.PlaySound(entry.handle, randomVol);

    // ランタイム状態を更新
    cue.lastPlayTime = currentTime;
    cue.currentConcurrent++;

    return entry.handle;
}

int SoundBank::FindCue(const gx::String& name) const
{
    for (int i = 0; i < static_cast<int>(m_cues.size()); ++i)
    {
        if (m_cues[i].name == name)
            return i;
    }
    return -1;
}

void SoundBank::Update(float deltaTime)
{
    // 時間経過で同時再生カウントを減少させる
    // 同時再生追跡用に各再生は約0.1秒続くものとする
    for (auto& cue : m_cues)
    {
        if (cue.currentConcurrent > 0)
        {
            // 同時再生カウントを段階的に減少
            // これは簡易実装；本来は個別のボイスを追跡すべき
            cue.currentConcurrent = (std::max)(0, cue.currentConcurrent - 1);
        }
    }
}

void SoundBank::Clear()
{
    m_cues.clear();
}

int SoundBank::SelectEntry(SoundCue& cue)
{
    if (cue.entries.empty())
        return -1;

    int count = static_cast<int>(cue.entries.size());

    switch (cue.mode)
    {
    case SoundCueMode::Random:
    {
        // 重み付きランダム選択
        float totalWeight = 0.0f;
        for (const auto& entry : cue.entries)
            totalWeight += entry.weight;

        if (totalWeight <= 0.0f)
            return 0;

        float roll = RandomFloat(0.0f, totalWeight);
        float accumulated = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            accumulated += cue.entries[i].weight;
            if (roll <= accumulated)
                return i;
        }
        return count - 1;
    }

    case SoundCueMode::Sequential:
    {
        int idx = cue.sequentialIndex;
        cue.sequentialIndex = (cue.sequentialIndex + 1) % count;
        return idx;
    }

    case SoundCueMode::Shuffle:
    {
        // 必要に応じてシャッフル順を初期化
        if (cue.shuffleOrder.empty() || static_cast<int>(cue.shuffleOrder.size()) != count)
        {
            InitializeShuffle(cue);
        }

        // シャッフルを使い切った場合、再シャッフル
        if (cue.shuffleIndex >= count)
        {
            InitializeShuffle(cue);
        }

        int idx = cue.shuffleOrder[cue.shuffleIndex];
        cue.shuffleIndex++;
        return idx;
    }
    }

    return 0;
}

float SoundBank::RandomFloat(float minVal, float maxVal) const
{
    if (minVal >= maxVal)
        return minVal;

    thread_local static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng);
}

void SoundBank::InitializeShuffle(SoundCue& cue)
{
    int count = static_cast<int>(cue.entries.size());
    cue.shuffleOrder.resize(count);
    std::iota(cue.shuffleOrder.begin(), cue.shuffleOrder.end(), 0);

    thread_local static std::mt19937 rng(std::random_device{}());
    std::shuffle(cue.shuffleOrder.begin(), cue.shuffleOrder.end(), rng);

    cue.shuffleIndex = 0;
}

} // namespace gx
