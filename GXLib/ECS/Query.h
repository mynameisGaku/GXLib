#pragma once
/// @file Query.h
/// @brief ECSクエリ（指定したコンポーネントを持つエンティティを一括処理）
///
/// Query<Position, Velocity> のようにテンプレートで型を指定すると、
/// そのコンポーネントを持つ全エンティティに対してForEach()でループできる。
/// ゲームロジックの更新処理の中心となるクラス。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"
#include "ECS/ECSTypes.h"

namespace gx { namespace ecs {

class World;

/// @brief コンポーネントTs...を持つすべてのエンティティを反復する型付きクエリ
///
/// 使用例:
///   Query<Position, Velocity> q(&world);
///   q.ForEach([](EntityID id, Position& pos, Velocity& vel) {
///       pos.x += vel.dx * dt;
///   });
template<typename... Ts>
class Query
{
public:
    explicit Query(World* world) : m_world(world) {}

    /// @brief このクエリに一致するすべてのエンティティに対してコールバックを実行する
    void ForEach(std::function<void(EntityID, Ts&...)> fn);

    /// @brief このクエリに一致するエンティティ数を返す
    uint32_t Count() const;

private:
    World* m_world;  ///< クエリ対象のECSワールド
};

}} // namespace gx::ecs
/// @}
