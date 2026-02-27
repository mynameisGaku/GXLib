#pragma once
/// @file Components.h
/// @brief Graphics非依存のビルトインコンポーネント
///
/// ParticleSystem, AudioSource, Script など、
/// Graphics モジュールに依存しないコンポーネント群。
/// Graphics依存コンポーネントは Graphics/3D/GraphicsComponents.h を参照。

#include "pch_common.h"
#include "Core/Scene/Component.h"

namespace gx
{

/// @brief パーティクルシステムコンポーネント
struct ParticleSystemComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::ParticleSystem;
    ComponentType GetType() const override { return k_Type; }
};

/// @brief オーディオソースコンポーネント
struct AudioSourceComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::AudioSource;
    ComponentType GetType() const override { return k_Type; }
    int soundHandle = -1;
    bool playOnStart = false;
    bool loop = false;
};

/// @brief ユーザー定義ロジック用スクリプトコンポーネント
struct ScriptComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Script;
    ComponentType GetType() const override { return k_Type; }
    std::function<void(float)> onUpdate;
    std::function<void()> onStart;
    std::function<void()> onDestroy;
    bool started = false;
};

} // namespace gx
