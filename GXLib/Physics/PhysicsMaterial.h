#pragma once
/// @file PhysicsMaterial.h
/// @brief 物理マテリアル — 摩擦・反発・密度のパラメータ管理
///
/// 「氷」「コンクリート」「ゴム」などのプリセットを名前で登録・検索できる。
/// ボディ作成時に PhysicsMaterialParams を渡すことで、
/// 素材感のある滑り方や跳ね返りを手軽に設定できる。
/// @addtogroup grp_physics/// @{

#include "pch_common.h"

namespace gx
{

/// @brief 物理マテリアルプリセット種別
enum class PhysicsMaterialPreset
{
    Default,
    Metal,
    Wood,
    Rubber,
    Ice,
    Concrete,
    Glass,
    Mud
};

/// @brief 物理マテリアルパラメータ
struct PhysicsMaterialParams
{
    float friction       = 0.5f;  ///< 摩擦係数
    float restitution    = 0.3f;  ///< 反発係数
    float density        = 1.0f;  ///< 密度
    float linearDamping  = 0.05f; ///< 線形減衰
    float angularDamping = 0.05f; ///< 角減衰

    /// @brief プリセットからパラメータを生成する
    /// @param preset プリセット種別
    /// @return 対応するパラメータ
    static PhysicsMaterialParams FromPreset(PhysicsMaterialPreset preset);
};

/// @brief 物理マテリアルライブラリ（名前 → パラメータ管理）
class PhysicsMaterialLibrary
{
public:
    /// @brief シングルトン取得
    static PhysicsMaterialLibrary& Instance();

    /// @brief マテリアルを登録する
    /// @param name マテリアル名
    /// @param params パラメータ
    /// @return マテリアルID
    uint32_t Register(const std::string& name, const PhysicsMaterialParams& params);

    /// @brief 名前でマテリアルを検索する
    /// @param name マテリアル名
    /// @return 見つかった場合はパラメータへのポインタ、なければ nullptr
    const PhysicsMaterialParams* Find(const std::string& name) const;

    /// @brief IDでマテリアルを検索する
    /// @param id マテリアルID
    /// @return 見つかった場合はパラメータへのポインタ、なければ nullptr
    const PhysicsMaterialParams* FindById(uint32_t id) const;

    /// @brief 全プリセットを一括登録する
    void RegisterDefaults();

    /// @brief 登録済みマテリアル数を取得する
    size_t GetCount() const { return m_entries.size(); }

    /// @brief 全エントリをクリアする
    void Clear();

private:
    PhysicsMaterialLibrary() = default;

    struct Entry
    {
        std::string name;
        PhysicsMaterialParams params;
    };

    std::vector<Entry> m_entries;
    std::unordered_map<std::string, uint32_t> m_nameIndex;
};

} // namespace gx
/// @}
