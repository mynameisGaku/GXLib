/// @file AnimationEventDispatcher.cpp
/// @brief アニメーションイベントディスパッチャーの実装
#include "pch_graphics.h"
#include "Graphics/3D/AnimationEventDispatcher.h"

namespace gx
{

void AnimationEventDispatcher::RegisterHandler(const std::string& eventName, AnimationEventCallback callback)
{
    m_handlers[eventName] = std::move(callback);
}

void AnimationEventDispatcher::RegisterGlobalHandler(AnimationEventCallback callback)
{
    m_globalHandler = std::move(callback);
}

void AnimationEventDispatcher::UnregisterHandler(const std::string& eventName)
{
    m_handlers.erase(eventName);
}

void AnimationEventDispatcher::ClearAllHandlers()
{
    m_handlers.clear();
    m_globalHandler = nullptr;
}

bool AnimationEventDispatcher::HasHandler(const std::string& eventName) const
{
    return m_handlers.find(eventName) != m_handlers.end();
}

void AnimationEventDispatcher::Update(const AnimationClip* clip, float previousTime, float currentTime)
{
    if (!clip) return;

    // CollectEvents はループ境界を跨ぐケースも正しく処理する
    std::vector<const AnimationEvent*> events;
    clip->CollectEvents(previousTime, currentTime, events);

    for (const auto* evt : events)
    {
        bool handled = false;

        // 名前付きハンドラを検索
        auto it = m_handlers.find(evt->name);
        if (it != m_handlers.end() && it->second)
        {
            it->second(*evt);
            handled = true;
        }

        // グローバルハンドラ
        if (m_globalHandler)
        {
            m_globalHandler(*evt);
            handled = true;
        }

        if (handled)
        {
            ++m_firedCount;
        }
    }
}

} // namespace gx
