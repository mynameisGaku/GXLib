/// @file SpriteAnimator.cpp
/// @brief スプライトアニメーション管理の実装

#include "pch_graphics.h"
#include "Graphics/Rendering/SpriteAnimator.h"
#include <cmath>
#include <algorithm>

namespace gx
{

// ============================================================================
// クリップ管理
// ============================================================================

void SpriteAnimator::AddClip(const SpriteAnimClip& clip)
{
    m_clips[clip.name] = clip;
}

void SpriteAnimator::RemoveClip(const std::string& name)
{
    m_clips.erase(name);

    // 現在再生中のクリップが削除されたら再生停止
    if (m_currentClipName == name)
    {
        m_playing = false;
        m_currentClipName.clear();
        m_currentFrame = 0;
        m_frameTimer = 0.0f;
    }

    // 前クリップが削除されたら遷移を中止
    if (m_previousClipName == name)
    {
        m_transitioning = false;
        m_previousClipName.clear();
        m_blendAlpha = 0.0f;
    }
}

const SpriteAnimClip* SpriteAnimator::GetClip(const std::string& name) const
{
    auto it = m_clips.find(name);
    if (it != m_clips.end())
    {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 遷移管理
// ============================================================================

void SpriteAnimator::AddTransition(const SpriteAnimTransition& transition)
{
    m_transitions.push_back(transition);
}

// ============================================================================
// 再生制御
// ============================================================================

void SpriteAnimator::Play(const std::string& clipName)
{
    auto it = m_clips.find(clipName);
    if (it == m_clips.end())
    {
        return; // 存在しないクリップは無視
    }

    // 同じクリップが再生中で遷移中でなければリセット再生
    if (m_currentClipName == clipName && !m_transitioning)
    {
        m_frameTimer = 0.0f;
        m_currentFrame = it->second.startFrame;
        m_playing = true;
        return;
    }

    // 遷移ルールが存在するか検索
    const SpriteAnimTransition* foundTransition = nullptr;
    if (!m_currentClipName.empty())
    {
        for (const auto& t : m_transitions)
        {
            if (t.fromClip == m_currentClipName && t.toClip == clipName)
            {
                foundTransition = &t;
                break;
            }
        }
    }

    if (foundTransition && foundTransition->blendDuration > 0.0f)
    {
        // クロスフェード開始
        m_previousClipName = m_currentClipName;
        m_prevFrameTimer = m_frameTimer;
        m_previousFrame = m_currentFrame;

        m_currentClipName = clipName;
        m_frameTimer = 0.0f;
        m_currentFrame = it->second.startFrame;

        m_blendTimer = 0.0f;
        m_blendDuration = foundTransition->blendDuration;
        m_blendAlpha = 0.0f;
        m_transitioning = true;
    }
    else
    {
        // 即切替
        m_currentClipName = clipName;
        m_frameTimer = 0.0f;
        m_currentFrame = it->second.startFrame;
        m_transitioning = false;
        m_blendAlpha = 0.0f;
        m_previousClipName.clear();
    }

    m_playing = true;
}

// ============================================================================
// フレーム計算
// ============================================================================

uint32_t SpriteAnimator::ComputeFrame(const SpriteAnimClip& clip, float timer) const
{
    if (clip.endFrame <= clip.startFrame)
    {
        return clip.startFrame;
    }

    uint32_t frameCount = clip.endFrame - clip.startFrame + 1;
    int frameIndex = static_cast<int>(std::floor(timer));

    if (clip.loop)
    {
        // ループ: modulo
        frameIndex = frameIndex % static_cast<int>(frameCount);
        if (frameIndex < 0)
        {
            frameIndex += static_cast<int>(frameCount);
        }
    }
    else
    {
        // 非ループ: クランプ
        frameIndex = std::max(0, std::min(frameIndex, static_cast<int>(frameCount) - 1));
    }

    return clip.startFrame + static_cast<uint32_t>(frameIndex);
}

float SpriteAnimator::ComputeNormalizedTime(const SpriteAnimClip& clip, float timer) const
{
    if (clip.endFrame <= clip.startFrame)
    {
        return 1.0f;
    }

    uint32_t frameCount = clip.endFrame - clip.startFrame + 1;
    float totalDuration = static_cast<float>(frameCount);

    if (totalDuration <= 0.0f)
    {
        return 1.0f;
    }

    float normalized = timer / totalDuration;
    return std::max(0.0f, std::min(normalized, 1.0f));
}

// ============================================================================
// 更新
// ============================================================================

void SpriteAnimator::Update(float deltaTime)
{
    if (!m_playing || m_currentClipName.empty())
    {
        return;
    }

    auto currentIt = m_clips.find(m_currentClipName);
    if (currentIt == m_clips.end())
    {
        m_playing = false;
        return;
    }

    const SpriteAnimClip& currentClip = currentIt->second;

    // フレームタイマーを進める（fps * speed * direction）
    float effectiveSpeed = currentClip.fps * currentClip.speed * m_speed
                           * static_cast<float>(m_direction);
    float timerDelta = deltaTime * effectiveSpeed;
    m_frameTimer += timerDelta;

    uint32_t frameCount = (currentClip.endFrame >= currentClip.startFrame)
                              ? (currentClip.endFrame - currentClip.startFrame + 1)
                              : 1;

    // 非ループで末尾到達チェック
    if (!currentClip.loop)
    {
        float maxTimer = static_cast<float>(frameCount);
        if (m_direction > 0 && m_frameTimer >= maxTimer)
        {
            m_frameTimer = maxTimer - 0.001f; // 末尾フレームに留まる
            m_playing = false;
        }
        else if (m_direction < 0 && m_frameTimer < 0.0f)
        {
            m_frameTimer = 0.0f;
            m_playing = false;
        }
    }

    // 現在フレーム計算
    uint32_t prevFrame = m_currentFrame;
    m_currentFrame = ComputeFrame(currentClip, m_frameTimer);

    // イベント発火チェック（フレームが変わったとき、途中フレームのイベントも発火）
    if (m_currentFrame != prevFrame)
    {
        // 方向を考慮してprevFrameからm_currentFrameの範囲のイベントをすべて発火
        uint32_t lo = (prevFrame < m_currentFrame) ? prevFrame + 1 : m_currentFrame;
        uint32_t hi = (prevFrame < m_currentFrame) ? m_currentFrame : prevFrame - 1;
        for (const auto& evt : currentClip.events)
        {
            if (evt.frame >= lo && evt.frame <= hi && evt.callback)
            {
                evt.callback();
            }
        }
    }

    // 自動遷移チェック（遷移中でないとき）
    if (!m_transitioning)
    {
        float normalizedTime = ComputeNormalizedTime(currentClip, m_frameTimer);
        for (const auto& t : m_transitions)
        {
            if (t.fromClip == m_currentClipName && normalizedTime >= t.exitTime)
            {
                // 遷移先クリップが存在するか確認
                auto destIt = m_clips.find(t.toClip);
                if (destIt != m_clips.end())
                {
                    if (t.blendDuration > 0.0f)
                    {
                        // クロスフェード開始
                        m_previousClipName = m_currentClipName;
                        m_prevFrameTimer = m_frameTimer;
                        m_previousFrame = m_currentFrame;

                        m_currentClipName = t.toClip;
                        m_frameTimer = 0.0f;
                        m_currentFrame = destIt->second.startFrame;

                        m_blendTimer = 0.0f;
                        m_blendDuration = t.blendDuration;
                        m_blendAlpha = 0.0f;
                        m_transitioning = true;
                    }
                    else
                    {
                        // 即切替
                        m_currentClipName = t.toClip;
                        m_frameTimer = 0.0f;
                        m_currentFrame = destIt->second.startFrame;
                    }
                    break;
                }
            }
        }
    }

    // クロスフェード更新
    if (m_transitioning)
    {
        m_blendTimer += deltaTime;
        if (m_blendDuration > 0.0f)
        {
            m_blendAlpha = std::min(m_blendTimer / m_blendDuration, 1.0f);
        }
        else
        {
            m_blendAlpha = 1.0f;
        }

        // 前クリップのフレーム更新
        auto prevIt = m_clips.find(m_previousClipName);
        if (prevIt != m_clips.end())
        {
            const SpriteAnimClip& prevClip = prevIt->second;
            float prevSpeed = prevClip.fps * prevClip.speed * m_speed
                              * static_cast<float>(m_direction);
            m_prevFrameTimer += deltaTime * prevSpeed;
            m_previousFrame = ComputeFrame(prevClip, m_prevFrameTimer);
        }

        // クロスフェード完了
        if (m_blendAlpha >= 1.0f)
        {
            m_transitioning = false;
            m_blendAlpha = 0.0f;
            m_previousClipName.clear();
            m_previousFrame = 0;
        }
    }
}

} // namespace gx
