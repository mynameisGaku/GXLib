#pragma once
/// @file Scene.h
/// @brief フレームワーク用のシーンインターフェース。最低限の入り口だけを定義する。

namespace GXFW
{

struct SceneContext;

/// @brief ユーザー定義シーン（Update/Render だけ実装）。必要な処理だけ書けるようにする。
class Scene
{
public:
    virtual ~Scene() = default;

    virtual const char* GetName() const = 0;

    virtual void OnEnter(SceneContext& ctx) { (void)ctx; }
    virtual void OnExit(SceneContext& ctx) { (void)ctx; }
    virtual void Update(SceneContext& ctx, float dt) { (void)ctx; (void)dt; }
    virtual void Render(SceneContext& ctx) { (void)ctx; }
    virtual void RenderUI(SceneContext& ctx) { (void)ctx; }
};

} // namespace GXFW
