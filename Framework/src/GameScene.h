#pragma once
/// @file GameScene.h
/// @brief シンプルなGameObject管理付きシーン。追加や更新の枠組みをまとめる。

#include "Scene.h"
#include "GameObject.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace GXFW
{

class GameScene : public Scene
{
public:
    GameScene() = default;
    ~GameScene() override = default;

    template <typename T, typename... Args>
    T* AddObject(Args&&... args)
    {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = obj.get();
        QueueAdd(std::move(obj));
        return raw;
    }

    void ClearObjects();

protected:
    void OnEnter(SceneContext& ctx) override final;
    void OnExit(SceneContext& ctx) override final;
    void Update(SceneContext& ctx, float dt) override final;
    void Render(SceneContext& ctx) override final;
    void RenderUI(SceneContext& ctx) override final;

    // ユーザーシーン用フック。継承先で必要な処理だけ実装する。
    virtual void OnSceneEnter(SceneContext& ctx) { (void)ctx; }
    virtual void OnSceneExit(SceneContext& ctx) { (void)ctx; }
    virtual void OnSceneUpdate(SceneContext& ctx, float dt) { (void)ctx; (void)dt; }
    virtual void OnSceneRender(SceneContext& ctx) { (void)ctx; }
    virtual void OnSceneRenderUI(SceneContext& ctx) { (void)ctx; }

    SceneContext* GetContext() const { return m_ctx; }

private:
    void QueueAdd(std::unique_ptr<GameObject> obj);
    void FlushPendingAdds();
    void UpdateObjects(float dt);
    void RenderObjects();
    void RenderUIObjects();
    void DestroyPending();

    SceneContext* m_ctx = nullptr;
    std::vector<std::unique_ptr<GameObject>> m_objects;
    std::vector<std::unique_ptr<GameObject>> m_pendingAdd;
};

} // namespace GXFW
