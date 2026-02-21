#pragma once
#include "Collision2D.h"

namespace GX {

/// @brief クアッドツリー（2D空間分割）テンプレート
///
/// 2D空間を4分割して再帰的にオブジェクトを管理する。
/// 大量の2Dオブジェクトの衝突判定を高速化するのに使う。
/// テンプレート引数Tはオブジェクトの識別子型。
template <typename T>
class Quadtree
{
public:
    /// @brief クアッドツリーを構築する
    /// @param bounds 管理する空間全体のAABB
    /// @param maxDepth 最大分割深度（デフォルト: 8）
    /// @param maxObjects ノードあたりの最大オブジェクト数（超えると分割、デフォルト: 8）
    Quadtree(const AABB2D& bounds, int maxDepth = 8, int maxObjects = 8)
        : m_maxDepth(maxDepth), m_maxObjects(maxObjects)
    {
        m_root = std::make_unique<Node>();
        m_root->bounds = bounds;
    }

    /// @brief オブジェクトを挿入する
    /// @param object 挿入するオブジェクト識別子
    /// @param bounds オブジェクトのAABB
    void Insert(const T& object, const AABB2D& bounds)
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
    void Query(const AABB2D& area, std::vector<T>& results) const
    {
        QueryNode(*m_root, area, results);
    }

    /// @brief 円範囲内のオブジェクトを検索する
    /// @param area 検索範囲の円
    /// @param results 見つかったオブジェクトの出力先
    void Query(const Circle& area, std::vector<T>& results) const
    {
        AABB2D circleBounds(
            { area.center.x - area.radius, area.center.y - area.radius },
            { area.center.x + area.radius, area.center.y + area.radius }
        );
        QueryNodeCircle(*m_root, area, circleBounds, results);
    }

    /// @brief 衝突の可能性があるオブジェクトペアを全て取得する
    /// @param pairs 衝突候補ペアの出力先
    void GetPotentialPairs(std::vector<std::pair<T, T>>& pairs) const
    {
        std::vector<std::pair<T, AABB2D>> ancestors;
        GetPairsFromNode(*m_root, ancestors, pairs);
    }

    /// @brief 登録されているオブジェクト数を取得する
    /// @return 全ノードの合計オブジェクト数
    int GetObjectCount() const
    {
        return CountObjects(*m_root);
    }

private:
    struct Node {
        AABB2D bounds;
        std::vector<std::pair<T, AABB2D>> objects;
        std::unique_ptr<Node> children[4]; // 左上/右上/左下/右下
        bool IsLeaf() const { return children[0] == nullptr; }
    };

    void Subdivide(Node& node)
    {
        Vector2 center = node.bounds.Center();
        Vector2 lo = node.bounds.min;
        Vector2 hi = node.bounds.max;

        node.children[0] = std::make_unique<Node>(); // 左上
        node.children[0]->bounds = { { lo.x, center.y }, { center.x, hi.y } };
        node.children[1] = std::make_unique<Node>(); // 右上
        node.children[1]->bounds = { center, hi };
        node.children[2] = std::make_unique<Node>(); // 左下
        node.children[2]->bounds = { lo, center };
        node.children[3] = std::make_unique<Node>(); // 右下
        node.children[3]->bounds = { { center.x, lo.y }, { hi.x, center.y } };
    }

    void InsertIntoNode(Node& node, const T& object, const AABB2D& bounds, int depth)
    {
        if (!Collision2D::TestAABBvsAABB(node.bounds, bounds)) return;

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
            if (child && Collision2D::TestAABBvsAABB(child->bounds, bounds))
                InsertIntoNode(*child, object, bounds, depth + 1);
        }
    }

    void RemoveFromNode(Node& node, const T& object)
    {
        auto it = std::remove_if(node.objects.begin(), node.objects.end(),
            [&](const std::pair<T, AABB2D>& p) { return p.first == object; });
        node.objects.erase(it, node.objects.end());

        if (!node.IsLeaf())
        {
            for (auto& child : node.children)
                if (child) RemoveFromNode(*child, object);
        }
    }

    void QueryNode(const Node& node, const AABB2D& area, std::vector<T>& results) const
    {
        if (!Collision2D::TestAABBvsAABB(node.bounds, area)) return;

        for (const auto& [obj, bnd] : node.objects)
        {
            if (Collision2D::TestAABBvsAABB(bnd, area))
                results.push_back(obj);
        }

        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) QueryNode(*child, area, results);
        }
    }

    void QueryNodeCircle(const Node& node, const Circle& circle, const AABB2D& circleBounds, std::vector<T>& results) const
    {
        if (!Collision2D::TestAABBvsAABB(node.bounds, circleBounds)) return;

        for (const auto& [obj, bnd] : node.objects)
        {
            if (Collision2D::TestAABBvsCircle(bnd, circle))
                results.push_back(obj);
        }

        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) QueryNodeCircle(*child, circle, circleBounds, results);
        }
    }

    void GetPairsFromNode(const Node& node, std::vector<std::pair<T, AABB2D>>& ancestors,
                          std::vector<std::pair<T, T>>& pairs) const
    {
        // 祖先ノードのオブジェクトと当たり判定
        for (const auto& [obj, bnd] : node.objects)
        {
            for (const auto& [ancObj, ancBnd] : ancestors)
            {
                if (Collision2D::TestAABBvsAABB(bnd, ancBnd))
                    pairs.push_back({ ancObj, obj });
            }
        }

        // このノード内のオブジェクト同士を判定
        for (size_t i = 0; i < node.objects.size(); ++i)
        {
            for (size_t j = i + 1; j < node.objects.size(); ++j)
            {
                if (Collision2D::TestAABBvsAABB(node.objects[i].second, node.objects[j].second))
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

    int CountObjects(const Node& node) const
    {
        int count = static_cast<int>(node.objects.size());
        if (!node.IsLeaf())
        {
            for (const auto& child : node.children)
                if (child) count += CountObjects(*child);
        }
        return count;
    }

    std::unique_ptr<Node> m_root;
    int m_maxDepth;
    int m_maxObjects;
};

} // namespace GX
