#pragma once
/// @file System.h
/// @brief ECSシステムの基底クラス
///
/// Update()をオーバーライドして毎フレームの処理を実装する。
/// World::AddSystem<T>()で登録し、World::UpdateSystems(dt)でまとめて実行される。
/// 物理・アニメーション・AIなど役割ごとにシステムに分けるのが基本パターン。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"

namespace gx { namespace ecs {

class World;

/// @brief 毎フレームエンティティを処理するECSシステムの基底クラス
///
/// これを継承しUpdate()をオーバーライドしてロジックを実装する。
/// システムはWorld::AddSystem<T>()で登録され、
/// World::UpdateSystems(dt)で実行される。
class System
{
public:
    virtual ~System() = default;

    /// @brief World::UpdateSystemsから毎フレーム呼び出される
    /// @param world     クエリ/変更するECSワールド
    /// @param deltaTime フレームのデルタタイム（秒）
    virtual void Update(World& world, float deltaTime) = 0;

    /// @brief このシステムのデバッグ/プロファイリング名
    virtual const char* GetName() const = 0;

    /// @brief システムの有効/無効を設定する
    /// @param enabled trueで有効化
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    /// @brief システムが有効かどうかを取得する
    /// @return 有効な場合true
    bool IsEnabled() const { return m_enabled; }

    /// @brief 実行優先度を取得する（低い値ほど先に実行）
    /// @return 優先度値
    int GetPriority() const { return m_priority; }
    /// @brief 実行優先度を設定する
    /// @param p 優先度値（低い値ほど先に実行）
    void SetPriority(int p) { m_priority = p; }

private:
    bool m_enabled = true;
    int m_priority = 0;      ///< 低い優先度が先に実行される
};

}} // namespace gx::ecs
/// @}
