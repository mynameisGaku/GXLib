#pragma once
/// @file BootScene.h
/// @brief 既定シーン（他が未設定のときに表示）。起動直後の挙動を確認できる。

#include "GameScene.h"

namespace GXFW
{

class BootScene : public GameScene
{
public:
    const char* GetName() const override { return "BootScene"; }

protected:
    void OnSceneEnter(SceneContext& ctx) override;
    void OnSceneUpdate(SceneContext& ctx, float dt) override;
    void OnSceneRenderUI(SceneContext& ctx) override;

private:
    float m_time = 0.0f;
};

} // namespace GXFW
