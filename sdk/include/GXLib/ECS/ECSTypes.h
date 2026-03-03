#pragma once
/// @file ECSTypes.h
/// @brief データ指向ECSの基本型と定数
///
/// EntityID・ComponentID・ComponentMaskなど、ECS全体で使う
/// 識別子型と定数をまとめて定義する。ECSの他のヘッダから参照される。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"

namespace gx { namespace ecs {

/// @brief エンティティ識別子（0は無効/null）
using EntityID = uint32_t;

/// @brief コンポーネント型識別子
using ComponentID = uint32_t;

/// @brief 無効なエンティティを示すセンチネル値
static constexpr EntityID k_InvalidEntity = 0;

/// @brief コンポーネント型の最大種類数
static constexpr uint32_t k_MaxComponents = 64;

/// @brief エンティティまたはアーキタイプが持つコンポーネントを表すビットマスク
using ComponentMask = uint64_t;

/// @brief エンティティごとの管理レコード
struct EntityRecord
{
    EntityID id = k_InvalidEntity;          ///< エンティティのID
    uint32_t archetypeIndex = UINT32_MAX;   ///< Worldのアーキタイプリストへのインデックス
    uint32_t rowIndex = UINT32_MAX;         ///< アーキタイプ内の行
    bool alive = false;                     ///< このエンティティが現在生存しているか
};

}} // namespace gx::ecs
/// @}
