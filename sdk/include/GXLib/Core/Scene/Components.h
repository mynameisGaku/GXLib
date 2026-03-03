#pragma once
/// @file Components.h
/// @brief Graphics非依存のビルトインコンポーネント
///
/// ParticleSystem, AudioSource, Script など、
/// Graphics モジュールに依存しないコンポーネント群。
/// Graphics依存コンポーネントは Graphics/3D/GraphicsComponents.h を参照。

#include "pch_common.h"
#include "Core/Scene/Component.h"
#include "Math/Vector3.h"

namespace gx
{
/// @addtogroup grp_scene
/// @{

/// @brief トランスフォームコンポーネント（コンポーネントベースの位置情報）
struct TransformComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Transform;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<TransformComponent>();
        c->position = position; c->rotation = rotation; c->scale = scale;
        return c;
    }
    Vector3 position;
    Vector3 rotation;
    Vector3 scale{ 1.0f, 1.0f, 1.0f };
};

/// @brief 名前コンポーネント（エンティティに追加の名前ラベルを付与）
struct NameComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Custom;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<NameComponent>(); c->name = name; return c;
    }
    gx::String name;
};

/// @brief タグコンポーネント（エンティティにタグ文字列を付与）
struct TagComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Custom;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<TagComponent>(); c->tag = tag; return c;
    }
    gx::String tag;
};

/// @brief パーティクルシステムコンポーネント
struct ParticleSystemComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::ParticleSystem;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override { return std::make_unique<ParticleSystemComponent>(); }
};

/// @brief オーディオソースコンポーネント
struct AudioSourceComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::AudioSource;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<AudioSourceComponent>();
        c->soundHandle = soundHandle; c->playOnStart = playOnStart; c->loop = loop;
        return c;
    }
    int soundHandle = -1;
    bool playOnStart = false;
    bool loop = false;
};

/// @brief ユーザー定義ロジック用スクリプトコンポーネント
struct ScriptComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Script;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<ScriptComponent>();
        c->onUpdate = onUpdate; c->onStart = onStart; c->onDestroy = onDestroy;
        c->started = started;
        return c;
    }
    std::function<void(float)> onUpdate;
    std::function<void()> onStart;
    std::function<void()> onDestroy;
    bool started = false;
};

/// @}
} // namespace gx
