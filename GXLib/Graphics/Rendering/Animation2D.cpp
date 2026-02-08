/// @file Animation2D.cpp
/// @brief 2Dアニメーションの実装
#include "pch.h"
#include "Graphics/Rendering/Animation2D.h"

namespace GX
{

void Animation2D::AddFrames(const int* handles, int count, float frameDuration)
{
    for (int i = 0; i < count; ++i)
    {
        m_frames.push_back({ handles[i], frameDuration });
    }
}

void Animation2D::Update(float deltaTime)
{
    if (m_frames.empty() || m_finished)
        return;

    m_timer += deltaTime * m_speed;

    while (m_timer >= m_frames[m_currentFrame].duration)
    {
        m_timer -= m_frames[m_currentFrame].duration;
        m_currentFrame++;

        if (m_currentFrame >= static_cast<int>(m_frames.size()))
        {
            if (m_loop)
            {
                m_currentFrame = 0;
            }
            else
            {
                m_currentFrame = static_cast<int>(m_frames.size()) - 1;
                m_finished = true;
                m_timer = 0.0f;
                return;
            }
        }
    }
}

int Animation2D::GetCurrentHandle() const
{
    if (m_frames.empty())
        return -1;
    return m_frames[m_currentFrame].handle;
}

void Animation2D::Reset()
{
    m_currentFrame = 0;
    m_timer = 0.0f;
    m_finished = false;
}

} // namespace GX
