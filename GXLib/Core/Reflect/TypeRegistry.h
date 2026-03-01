#pragma once
/// @file TypeRegistry.h
/// @brief リフレクション対象型のグローバル登録・検索
///
/// GX_REFLECT_BEGIN マクロで登録された型をシングルトンで一元管理する。
/// 名前やハッシュで TypeInfo を検索でき、エディタやスクリプト層から
/// 任意の型情報にアクセスする際の起点となる。
/// @addtogroup grp_reflection/// @{

#include "pch_common.h"
#include "Core/Reflect/TypeInfo.h"

namespace gx
{

/// @brief 全TypeInfoインスタンスを所有するシングルトンレジストリ
class TypeRegistry
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return TypeRegistry の唯一のインスタンス
    static TypeRegistry& Instance();

    /// @brief 型を登録する（所有権を移譲）
    /// @param type 登録する TypeInfo（unique_ptr で所有権を渡す）
    void RegisterType(std::unique_ptr<TypeInfo> type);

    /// @brief 名前で型を検索する（見つからない場合nullptrを返す）
    /// @param name 検索する型名
    /// @return 見つかった TypeInfo へのポインタ。見つからない場合は nullptr
    const TypeInfo* FindType(const std::string& name) const;

    /// @brief ハッシュで型を検索する（見つからない場合nullptrを返す）
    /// @param hash 検索するハッシュ値
    /// @return 見つかった TypeInfo へのポインタ。見つからない場合は nullptr
    const TypeInfo* FindTypeByHash(uint32_t hash) const;

    /// @brief 登録済みの全型を取得する
    /// @return TypeInfo ポインタの配列
    std::vector<const TypeInfo*> GetAllTypes() const;

    /// @brief 登録済み型の数を取得する
    /// @return 型の数
    uint32_t GetTypeCount() const { return static_cast<uint32_t>(m_types.size()); }

    /// @brief 登録済みの全型を削除する
    void Clear();

private:
    TypeRegistry() = default;
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    std::unordered_map<std::string, std::unique_ptr<TypeInfo>> m_types; ///< 名前→TypeInfo のマップ（所有）
    std::unordered_map<uint32_t, TypeInfo*> m_hashMap;                ///< ハッシュ→TypeInfo の逆引きマップ
};

} // namespace gx
/// @}
