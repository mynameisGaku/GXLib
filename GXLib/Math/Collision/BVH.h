#pragma once
#include "Collision3D.h"

namespace GX {

/// @brief BVH（境界ボリューム階層）テンプレート
///
/// 多数の3DオブジェクトをAABBの木構造で管理し、空間クエリを高速化する。
/// SAH（表面積ヒューリスティック）で最適な分割を行う。
/// テンプレート引数Tはオブジェクトの識別子型（int、ポインタなど）。
template <typename T>
class BVH
{
public:
    /// @brief オブジェクト群からBVHを構築する
    /// @param objects (識別子, AABB)のペア配列
    void Build(const std::vector<std::pair<T, AABB3D>>& objects)
    {
        m_objects = objects;
        m_nodes.clear();
        if (m_objects.empty()) return;

        std::vector<int> indices(m_objects.size());
        for (int i = 0; i < static_cast<int>(indices.size()); ++i)
            indices[i] = i;

        BuildRecursive(indices, 0, static_cast<int>(indices.size()));
    }

    /// @brief BVHをクリアする
    void Clear()
    {
        m_nodes.clear();
        m_objects.clear();
    }

    /// @brief AABB範囲内のオブジェクトを検索する
    /// @param area 検索範囲のAABB
    /// @param results 見つかったオブジェクトの出力先
    void Query(const AABB3D& area, std::vector<T>& results) const
    {
        if (m_nodes.empty()) return;
        QueryNode(0, area, results);
    }

    /// @brief レイと交差するオブジェクトを全て検索する
    /// @param ray 検索レイ
    /// @param results 見つかったオブジェクトの出力先
    void Query(const Ray& ray, std::vector<T>& results) const
    {
        if (m_nodes.empty()) return;
        QueryRayNode(0, ray, results);
    }

    /// @brief レイキャストで最も近いオブジェクトを取得する
    /// @param ray レイ
    /// @param outT ヒット位置のパラメータt
    /// @param outObject ヒットしたオブジェクトの出力先（nullptrで省略可）
    /// @return ヒットした場合true
    bool Raycast(const Ray& ray, float& outT, T* outObject = nullptr) const
    {
        if (m_nodes.empty()) return false;
        float closestT = 1e30f;
        int closestIdx = -1;
        RaycastNode(0, ray, closestT, closestIdx);
        if (closestIdx >= 0)
        {
            outT = closestT;
            if (outObject) *outObject = m_objects[closestIdx].first;
            return true;
        }
        return false;
    }

private:
    struct Node {
        AABB3D bounds;
        int left = -1, right = -1;
        int objectIndex = -1;
        bool IsLeaf() const { return objectIndex >= 0; }
    };

    int BuildRecursive(std::vector<int>& indices, int start, int end)
    {
        int nodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back({});
        Node& node = m_nodes[nodeIdx];

        // 境界を計算
        node.bounds = m_objects[indices[start]].second;
        for (int i = start + 1; i < end; ++i)
            node.bounds = node.bounds.Merged(m_objects[indices[i]].second);

        int count = end - start;
        if (count == 1)
        {
            node.objectIndex = indices[start];
            return nodeIdx;
        }

        // SAH（表面積ヒューリスティック）で最良の分割位置を探す
        // 分割後の「表面積 x 要素数」が小さいほどレイ探索が高速になる
        int bestAxis = 0;
        int bestSplit = start + count / 2;
        float bestCost = 1e30f;

        for (int axis = 0; axis < 3; ++axis)
        {
            // 軸に沿ってソート
            std::sort(indices.begin() + start, indices.begin() + end,
                [&](int a, int b) {
                    float ca = GetAxisCenter(m_objects[a].second, axis);
                    float cb = GetAxisCenter(m_objects[b].second, axis);
                    return ca < cb;
                });

            // 分割位置ごとのコストを評価
            for (int split = start + 1; split < end; ++split)
            {
                AABB3D leftBounds = m_objects[indices[start]].second;
                for (int i = start + 1; i < split; ++i)
                    leftBounds = leftBounds.Merged(m_objects[indices[i]].second);

                AABB3D rightBounds = m_objects[indices[split]].second;
                for (int i = split + 1; i < end; ++i)
                    rightBounds = rightBounds.Merged(m_objects[indices[i]].second);

                float cost = leftBounds.SurfaceArea() * (split - start) +
                             rightBounds.SurfaceArea() * (end - split);
                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestAxis = axis;
                    bestSplit = split;
                }
            }
        }

        // 最良軸で最終ソート
        std::sort(indices.begin() + start, indices.begin() + end,
            [&](int a, int b) {
                return GetAxisCenter(m_objects[a].second, bestAxis) <
                       GetAxisCenter(m_objects[b].second, bestAxis);
            });

        // 再帰後にインデックスを書き戻す（vector再確保対策）
        int leftIdx = BuildRecursive(indices, start, bestSplit);
        m_nodes[nodeIdx].left = leftIdx;
        int rightIdx = BuildRecursive(indices, bestSplit, end);
        m_nodes[nodeIdx].right = rightIdx;
        return nodeIdx;
    }

    float GetAxisCenter(const AABB3D& bounds, int axis) const
    {
        Vector3 c = bounds.Center();
        return (axis == 0) ? c.x : ((axis == 1) ? c.y : c.z);
    }

    void QueryNode(int nodeIdx, const AABB3D& area, std::vector<T>& results) const
    {
        const Node& node = m_nodes[nodeIdx];
        if (!Collision3D::TestAABBVsAABB(node.bounds, area)) return;

        if (node.IsLeaf())
        {
            if (Collision3D::TestAABBVsAABB(m_objects[node.objectIndex].second, area))
                results.push_back(m_objects[node.objectIndex].first);
            return;
        }

        if (node.left >= 0)  QueryNode(node.left, area, results);
        if (node.right >= 0) QueryNode(node.right, area, results);
    }

    void QueryRayNode(int nodeIdx, const Ray& ray, std::vector<T>& results) const
    {
        const Node& node = m_nodes[nodeIdx];
        float t;
        if (!Collision3D::RaycastAABB(ray, node.bounds, t)) return;

        if (node.IsLeaf())
        {
            if (Collision3D::RaycastAABB(ray, m_objects[node.objectIndex].second, t))
                results.push_back(m_objects[node.objectIndex].first);
            return;
        }

        if (node.left >= 0)  QueryRayNode(node.left, ray, results);
        if (node.right >= 0) QueryRayNode(node.right, ray, results);
    }

    void RaycastNode(int nodeIdx, const Ray& ray, float& closestT, int& closestIdx) const
    {
        const Node& node = m_nodes[nodeIdx];
        float t;
        if (!Collision3D::RaycastAABB(ray, node.bounds, t)) return;
        if (t > closestT) return; // 早期終了

        if (node.IsLeaf())
        {
            if (Collision3D::RaycastAABB(ray, m_objects[node.objectIndex].second, t))
            {
                if (t < closestT)
                {
                    closestT = t;
                    closestIdx = node.objectIndex;
                }
            }
            return;
        }

        if (node.left >= 0)  RaycastNode(node.left, ray, closestT, closestIdx);
        if (node.right >= 0) RaycastNode(node.right, ray, closestT, closestIdx);
    }

    std::vector<Node> m_nodes;
    std::vector<std::pair<T, AABB3D>> m_objects;
};

} // namespace GX
