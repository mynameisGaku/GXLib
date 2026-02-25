#pragma once
/// @file EditorCommands.h
/// @brief エディタ操作コマンド（Transform変更、エンティティ追加/削除/リネーム/親子変更）

#include "UndoSystem.h"
#include "Scene/SceneGraph.h"
#include "Graphics/3D/Transform3D.h"

/// @brief Transform 変更コマンド
class TransformChangeCommand : public ICommand
{
public:
    TransformChangeCommand(SceneGraph& scene, int entityIdx,
                           const XMFLOAT3& oldPos, const XMFLOAT3& oldRot, const XMFLOAT3& oldScale,
                           const XMFLOAT3& newPos, const XMFLOAT3& newRot, const XMFLOAT3& newScale)
        : m_scene(scene), m_entityIdx(entityIdx)
        , m_oldPos(oldPos), m_oldRot(oldRot), m_oldScale(oldScale)
        , m_newPos(newPos), m_newRot(newRot), m_newScale(newScale)
    {}

    void Execute() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (!e) return;
        e->transform.SetPosition(m_newPos);
        e->transform.SetRotation(m_newRot);
        e->transform.SetScale(m_newScale);
    }

    void Undo() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (!e) return;
        e->transform.SetPosition(m_oldPos);
        e->transform.SetRotation(m_oldRot);
        e->transform.SetScale(m_oldScale);
    }

    std::string GetDescription() const override { return "Transform Change"; }

private:
    SceneGraph& m_scene;
    int m_entityIdx;
    XMFLOAT3 m_oldPos, m_oldRot, m_oldScale;
    XMFLOAT3 m_newPos, m_newRot, m_newScale;
};

/// @brief エンティティ追加コマンド（Execute で追加、Undo で削除）
class AddEntityCommand : public ICommand
{
public:
    AddEntityCommand(SceneGraph& scene, const std::string& name)
        : m_scene(scene), m_name(name)
    {}

    void Execute() override
    {
        m_createdIdx = m_scene.AddEntity(m_name);
        m_scene.selectedEntity = m_createdIdx;
    }

    void Undo() override
    {
        if (m_createdIdx >= 0)
        {
            m_scene.RemoveEntity(m_createdIdx);
            if (m_scene.selectedEntity == m_createdIdx)
                m_scene.selectedEntity = -1;
        }
    }

    std::string GetDescription() const override { return "Add Entity: " + m_name; }
    int GetCreatedIndex() const { return m_createdIdx; }

private:
    SceneGraph& m_scene;
    std::string m_name;
    int m_createdIdx = -1;
};

/// @brief エンティティ削除コマンド（Execute で削除マーク、Undo で復活は不可→選択解除のみ）
/// @note GPU リソースの関係で完全な Undo は困難。削除は名前のみ記録。
class RemoveEntityCommand : public ICommand
{
public:
    RemoveEntityCommand(SceneGraph& scene, int entityIdx)
        : m_scene(scene), m_entityIdx(entityIdx)
    {
        auto* e = m_scene.GetEntity(entityIdx);
        if (e)
        {
            m_savedName = e->name;
            m_savedTransform = e->transform;
            m_savedParentIndex = e->parentIndex;
        }
    }

    void Execute() override
    {
        m_scene.RemoveEntity(m_entityIdx);
        if (m_scene.selectedEntity == m_entityIdx)
            m_scene.selectedEntity = -1;
    }

    void Undo() override
    {
        // Re-create as empty entity (model data is lost)
        int newIdx = m_scene.AddEntity(m_savedName);
        auto* e = m_scene.GetEntity(newIdx);
        if (e)
        {
            e->transform = m_savedTransform;
            e->parentIndex = m_savedParentIndex;
        }
        m_scene.selectedEntity = newIdx;
        m_entityIdx = newIdx;
    }

    std::string GetDescription() const override { return "Remove Entity: " + m_savedName; }

private:
    SceneGraph& m_scene;
    int m_entityIdx;
    std::string m_savedName;
    GX::Transform3D m_savedTransform;
    int m_savedParentIndex = -1;
};

/// @brief リネームコマンド
class RenameEntityCommand : public ICommand
{
public:
    RenameEntityCommand(SceneGraph& scene, int entityIdx,
                        const std::string& oldName, const std::string& newName)
        : m_scene(scene), m_entityIdx(entityIdx), m_oldName(oldName), m_newName(newName)
    {}

    void Execute() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (e) e->name = m_newName;
    }

    void Undo() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (e) e->name = m_oldName;
    }

    std::string GetDescription() const override { return "Rename: " + m_oldName + " -> " + m_newName; }

private:
    SceneGraph& m_scene;
    int m_entityIdx;
    std::string m_oldName, m_newName;
};

/// @brief 親子関係変更コマンド
class ReparentCommand : public ICommand
{
public:
    ReparentCommand(SceneGraph& scene, int entityIdx, int oldParent, int newParent)
        : m_scene(scene), m_entityIdx(entityIdx), m_oldParent(oldParent), m_newParent(newParent)
    {}

    void Execute() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (e) e->parentIndex = m_newParent;
    }

    void Undo() override
    {
        auto* e = m_scene.GetEntity(m_entityIdx);
        if (e) e->parentIndex = m_oldParent;
    }

    std::string GetDescription() const override { return "Reparent Entity"; }

private:
    SceneGraph& m_scene;
    int m_entityIdx;
    int m_oldParent, m_newParent;
};
