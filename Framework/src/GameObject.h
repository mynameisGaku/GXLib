#pragma once
/// @file GameObject.h
/// @brief GameScene 向けのシンプルなオブジェクト単位。ライフサイクルの最小構成を提供する。

#include <cstddef>

namespace GXFW
{

struct SceneContext;
class GameScene;

class GameObject
{
public:
    virtual ~GameObject() = default;

    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    void Destroy() { m_pendingDestroy = true; }

protected:
    SceneContext& Ctx();

    virtual void OnStart() {}
    virtual void OnUpdate(float dt) { (void)dt; }
    virtual void OnRender() {}
    virtual void OnRenderUI() {}
    virtual void OnDestroy() {}

private:
    friend class GameScene;

    void Attach(SceneContext* ctx) { m_ctx = ctx; }
    void StartIfNeeded()
    {
        if (!m_started)
        {
            m_started = true;
            OnStart();
        }
    }
    bool IsPendingDestroy() const { return m_pendingDestroy; }

    SceneContext* m_ctx = nullptr;
    bool m_started = false;
    bool m_active = true;
    bool m_pendingDestroy = false;
};

} // namespace GXFW
