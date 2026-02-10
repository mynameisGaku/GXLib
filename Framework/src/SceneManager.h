#pragma once
/// @file SceneManager.h
/// @brief シーン切り替えの補助クラス。安全に遷移できるようにする。

#include <memory>
#include "Scene.h"

namespace GXFW
{

class SceneManager
{
public:
    void SetScene(std::unique_ptr<Scene> scene, SceneContext& ctx);
    void Update(SceneContext& ctx, float dt);
    void Render(SceneContext& ctx);
    void RenderUI(SceneContext& ctx);

private:
    void ApplyPending(SceneContext& ctx);

    std::unique_ptr<Scene> m_current;
    std::unique_ptr<Scene> m_pending;
    bool m_hasPending = false;
};

} // namespace GXFW
