#pragma once
#include "Collision3D.h"

namespace GX {

/// @brief オクツリー（3D空間分割）テンプレート
///
/// 3D空間を8分割して再帰的にオブジェクトを管理する。
/// 大量の3Dオブジェクトの衝突判定やカリングを高速化するのに使う。
/// 視錐台クエリにも対応しており、描画カリングにも利用可能。
/// テンプレート引数Tはオブジェクトの識別子型。
template <typename T>
class Octree
{
public:
    /// @brief オクツリーを構築する
    /// @param bounds 管理する空間全体のAABB
    /// @param maxDepth 最大分割深度（デフォルト: 8）
    /// @param maxObjects ノードあたりの最大オブジェクト数（超えると分割、デフォルト: 8）
    Octree(const AABB3D& bounds, int maxDepth = 8, int maxObjects = 8)
        : m_maxDepth(maxDepth), m_maxObjects(maxObjects)
    {
        m_root = std::make_unique<Node>();
        m_root->bounds = bounds;
    }

    /// @brief オブジェクトを挿入する
    /// @param object 挿入するオブジェクト識別子
    /// @param bounds オブジェクトのAABB
    void Insert(const T& object, const AABB3D& bounds)
    {
        InsertIntoNode(*m_root, object, bounds, 0);
    }

    /// @brief オブジェクトを削除する
    /// @param object 削除するオブジェクト識別子
    void Remove(const T& object)
    {
        RemoveFromNode(*m_root, object);
    }

    /// @brief 全オブジェクトを削除する
    void Clear()
    {
        m_root->objects.clear();
        for (auto& child : m_root->children)
            child.reset();
    }

    /// @brief AABB範囲内のオブジェクトを検索する
    /// @param area 検索範囲のAABB
    /// @param results 見つかったオブジェクトの出力先
    void Query(const AABB3D& area, std::vector<T>& results) const
    {
        QueryNode(*m_root, area, results);
    }

    /// @brief 球範囲内のオブジェクトを検索する
    /// @param area 検索範囲の球
    /// @param results 見つかったオブジェクトの出力先
    void Query(const Sphere& area, std::vector<T>& results) const
    {
        AABB3D sphereBounds(
            { area.center.x - area.radius, area.center.y - area.radius, area.center.z - area.radius },
            { area.center.x + area.radius, area.center.y + area.radius, area.center.z + area.radius }
        );
        QueryNodeSphere(*m_root, area, sphereBounds, results);
    }

    /// @brief 視錐台内のオブジェクトを検索する（カリング用）
    /// @param frustum 検索範囲の視錐台
    /// @param results 見つかったオブジェクトの出力先
    void Query(const Frustum& frustum, std::vector<T>& results) const
    {
        QueryNodeFrustum(*m_root, frustum, results);
    }

    /// @brief 衝突の可能性があるオブジェクトペアを全て取得する
    /// @param pairs 衝突候補ペアの出力先
    void GetPotentialPairs(std::vector<std::pair<T, T>>& pairs) const
    {
        std::vector<std::pair<T, AABB3D>> ancestors;
        GetPairsFromNode(*m_root, ancestors, pairs);
    }

private:
    struct Node {
        AABB3D bounds;
        std::vector<std::pair<T, AABB3D>> objects;
        std::unique_ptr<Node> children[8];
        bool IsLeaf() const { return children[0] == nullptr; }
    };

    void Subdivide(Node& node)
    {
        Vector3 center = node.bounds.Center();
        Vector3 lo = node.bounds.min;
        Vector3 hi = node.bounds.max;

        // 8分割（オクタント）
        for (int i = 0; i < 8; ++i)
        {
            node.children[i] = std::make_unique<Node>();
            node.children[i]->bounds.min = {
                (i & 1) ? center.x : lo.x,
                (i & 2) ? center.y : lo.y,
                (i & 4) ? center.z : lo.z
            };
            node.children[i]->bounds.max = {
                (i & 1) ? hi.x : center.x,
                (i & 2) ? hi.y : center.y,
                (i & 4) ? hi.z : center.z
            };
        }
    }

    void InsertIntoNode(Node& node, const T& object, const AABB3D& bounds, int depth)
    {
        if (!Collision3D::TestAABBVsAABB(node.bounds, bounds)) return;

        if (node.IsLeaf())
        {
            node.objects.push_back({ object, bounds });
            if (static_cast<int>(node.objects.size()) > m_maxObjects && depth < m_maxDepth)
            {
                Subdivide(node);
                auto objects = std::move(node.objects);
                node.objects.clear();
                for (auto& [obj, bnd] : objects)
                    InsertIntoNode(node, obj, bnd, depth);
            }
            return;
        }

        for (auto& child : node.children)
        {
            if (child && Collision3D::TestAABBVsAABB(child->bounds, bounds))
                InsertIntoNode(*child, object, bounds, depth + 1);
        }
    }

    void RemoveFromNode(Node& node, const T& object)
    {
        auto it = std::remove_if(node.objects.begin(), node.objects.end(),
            [&](const std::pair<T, AABB3D>& p) { return p.first == object; });
        node.objects.erase(it, node.objects.end());

        if (!node.IsLeaf())
        {
            for (auto& child : node.children)
                if (child) RemoveFromNode(*child, object);
        }
    }

    void QueryNode(const Node& node, const AABB3D& area, std::vector<T>& results) const
    {
        if (!Collision3D::TestAABBVsAABB(node.bounds, area)) return;

        for (const auto& [obj, bnd] : node.objects)
        {
            if (Collision3D::TestAABBVsAABB(bnd, area))
                results.push_back(obj);
        }

        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) QueryNode(*child, area, results);
        }
    }

    void QueryNodeSphere(const Node& node, const Sphere& sphere, const AABB3D& sphereBounds, std::vector<T>& results) const
    {
        if (!Collision3D::TestAABBVsAABB(node.bounds, sphereBounds)) return;

        for (const auto& [obj, bnd] : node.objects)
        {
            if (Collision3D::TestSphereVsAABB(sphere, bnd))
                results.push_back(obj);
        }

        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) QueryNodeSphere(*child, sphere, sphereBounds, results);
        }
    }

    void QueryNodeFrustum(const Node& node, const Frustum& frustum, std::vector<T>& results) const
    {
        if (!Collision3D::TestFrustumVsAABB(frustum, node.bounds)) return;

        for (const auto& [obj, bnd] : node.objects)
        {
            if (Collision3D::TestFrustumVsAABB(frustum, bnd))
                results.push_back(obj);
        }

        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) QueryNodeFrustum(*child, frustum, results);
        }
    }

    void GetPairsFromNode(const Node& node, std::vector<std::pair<T, AABB3D>>& ancestors,
                          std::vector<std::pair<T, T>>& pairs) const
    {
        for (const auto& [obj, bnd] : node.objects)
        {
            for (const auto& [ancObj, ancBnd] : ancestors)
            {
                if (Collision3D::TestAABBVsAABB(bnd, ancBnd))
                    pairs.push_back({ ancObj, obj });
            }
        }

        for (size_t i = 0; i < node.objects.size(); ++i)
        {
            for (size_t j = i + 1; j < node.objects.size(); ++j)
            {
                if (Collision3D::TestAABBVsAABB(node.objects[i].second, node.objects[j].second))
                    pairs.push_back({ node.objects[i].first, node.objects[j].first });
            }
        }

        if (!node.IsLeaf())
        {
            auto combined = ancestors;
            combined.insert(combined.end(), node.objects.begin(), node.objects.end());
            for (const auto& child : node.children)
                if (child) GetPairsFromNode(*child, combined, pairs);
        }
    }

    std::unique_ptr<Node> m_root;
    int m_maxDepth;
    int m_maxObjects;
};

} // namespace GX
