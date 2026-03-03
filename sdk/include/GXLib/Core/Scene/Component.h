#pragma once
/// @file Component.h
/// @brief コンポーネント基底クラスとコンポーネント種別定義

#include "pch_common.h"

namespace gx
{
/// @addtogroup grp_scene
/// @{

class Entity;

/// @brief コンポーネントの種類ID
enum class ComponentType : uint32_t
{
    Transform,
    MeshRenderer,
    SkinnedMeshRenderer,
    Camera,
    Light,
    ParticleSystem,
    AudioSource,
    Terrain,
    RigidBody,
    LOD,
    Script,
    Custom,
    _Count
};

/// @brief コンポーネント基底クラス
class Component
{
public:
    virtual ~Component() = default;
    virtual ComponentType GetType() const = 0;

    /// @brief コンポーネントの複製を作成する
    /// @return 複製されたコンポーネント。デフォルトは nullptr
    virtual std::unique_ptr<Component> Clone() const { return nullptr; }

    Entity* GetEntity() const { return m_entity; }
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool e) { m_enabled = e; }

protected:
    friend class Entity;
    Entity* m_entity = nullptr;
    bool m_enabled = true;
};

/// @}
} // namespace gx
