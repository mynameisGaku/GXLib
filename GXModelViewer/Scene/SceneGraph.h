#pragma once
/// @file SceneGraph.h
/// @brief Simple scene graph for GXModelViewer

#include <string>
#include <vector>
#include <memory>

#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"

/// @brief A single entity in the scene graph
struct SceneEntity
{
    std::string      name;
    GX::Transform3D  transform;
    GX::Model*       model = nullptr;                  // non-owning reference
    std::unique_ptr<GX::Model> ownedModel;             // ownership for imported models
    GX::Material     materialOverride;                  // per-entity material override
    bool             useMaterialOverride = false;
    int              parentIndex = -1;
    bool             visible = true;
    std::string      sourcePath;                       // import元ファイルパス
};

/// @brief Simple flat scene graph (entities stored in a vector)
class SceneGraph
{
public:
    /// Add a new entity with the given name.
    /// @return The index of the newly added entity.
    int AddEntity(const std::string& name);

    /// Remove an entity by index (marks slot as removed).
    void RemoveEntity(int index);

    /// Get a mutable pointer to the entity at the given index (nullptr if invalid).
    SceneEntity* GetEntity(int index);

    /// Get a const pointer to the entity at the given index (nullptr if invalid).
    const SceneEntity* GetEntity(int index) const;

    /// Get the full entity list (including removed slots).
    const std::vector<SceneEntity>& GetEntities() const { return m_entities; }

    /// Get the number of entity slots (including removed).
    int GetEntityCount() const { return static_cast<int>(m_entities.size()); }

    /// Currently selected entity index (-1 = none).
    int selectedEntity = -1;

private:
    std::vector<SceneEntity> m_entities;
    std::vector<int>         m_freeIndices;
};
