#pragma once
/// @file PrefabVariant.h
/// @brief プレハブの派生（バリアント）— テンプレートの継承と差分上書き
///
/// 基底プレハブを継承しつつ、一部コンポーネントのプロパティだけを
/// 上書きした派生テンプレートを定義できる。
/// 「ゾンビA」を基底に「ゾンビボス」を作るような使い方を想定。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"

namespace gx
{

/// @brief コンポーネント内の単一プロパティ上書き情報
struct PropertyOverride
{
    gx::String componentName; ///< 対象コンポーネント名
    gx::String propertyName;  ///< 対象プロパティ名
    gx::String value;         ///< 上書き値（文字列表現）
    bool isRemoved = false;    ///< true の場合、このプロパティを削除扱いにする
};

/// @brief コンポーネント単位の上書き情報
struct ComponentOverride
{
    gx::String componentName;                 ///< 対象コンポーネント名
    bool isAdded = false;                      ///< true の場合、バリアントで追加されたコンポーネント
    bool isRemoved = false;                    ///< true の場合、バリアントで削除されたコンポーネント
    gx::Vector<PropertyOverride> properties;  ///< プロパティ上書きの一覧
};

/// @brief 基底プレハブの定義データ（エンティティテンプレート）
struct PrefabData
{
    uint64_t prefabId = 0;                     ///< プレハブの一意ID
    gx::String name;                          ///< プレハブ名
    gx::Vector<uint8_t> entityData;           ///< シリアライズされたエンティティデータ
    gx::Vector<uint64_t> childPrefabIds;      ///< 子プレハブIDの一覧
};

/// @brief 基底プレハブから派生したバリアントの定義データ
struct PrefabVariantData
{
    uint64_t variantId = 0;       ///< バリアントの一意ID
    uint64_t basePrefabId = 0;    ///< 基底プレハブID
    gx::String name;             ///< バリアント名
    gx::Vector<ComponentOverride> overrides; ///< コンポーネント上書きの一覧
    gx::HashMap<uint32_t, gx::Vector<ComponentOverride>> childOverrides; ///< 子エンティティごとの上書き
};

/// @brief プレハブとバリアントの継承管理システム
///
/// ネスト（バリアントのバリアント）、プロパティ/コンポーネント単位の上書き、
/// ファイルへのシリアライズ/デシリアライズに対応する。
class PrefabVariantSystem
{
public:
    PrefabVariantSystem() = default;
    ~PrefabVariantSystem() = default;

    // --- プレハブ管理 ---

    /// @brief 新しいプレハブを登録する
    /// @param data 登録するプレハブデータ
    /// @return 割り当てられたプレハブID
    uint64_t RegisterPrefab(const PrefabData& data);

    /// @brief IDを指定してプレハブを登録解除する
    /// @param prefabId 登録解除するプレハブのID
    /// @return 見つかって削除された場合 true
    bool UnregisterPrefab(uint64_t prefabId);

    /// @brief IDでプレハブを取得する
    /// @param prefabId プレハブID
    /// @return プレハブデータへのポインタ（見つからない場合 nullptr）
    const PrefabData* GetPrefab(uint64_t prefabId) const;

    /// @brief 登録済みプレハブ数を取得する
    /// @return プレハブの数
    size_t GetPrefabCount() const;

    /// @brief 指定IDのプレハブが存在するか判定する
    /// @param prefabId 確認するプレハブID
    /// @return 存在する場合 true
    bool HasPrefab(uint64_t prefabId) const;

    // --- バリアント管理 ---

    /// @brief 基底プレハブからバリアントを作成する
    /// @param basePrefabId 基底プレハブのID
    /// @param name バリアント名
    /// @return バリアントID（失敗時は 0）
    uint64_t CreateVariant(uint64_t basePrefabId, const gx::String& name);

    /// @brief バリアントを削除する
    /// @param variantId 削除するバリアントのID
    /// @return 見つかって削除された場合 true
    bool DeleteVariant(uint64_t variantId);

    /// @brief IDでバリアントを取得する
    /// @param variantId バリアントID
    /// @return バリアントデータへのポインタ（見つからない場合 nullptr）
    const PrefabVariantData* GetVariant(uint64_t variantId) const;

    /// @brief 登録済みバリアント数を取得する
    /// @return バリアントの数
    size_t GetVariantCount() const;

    /// @brief 指定IDのバリアントが存在するか判定する
    /// @param variantId バリアントID
    /// @return 存在する場合 true
    bool HasVariant(uint64_t variantId) const;

    /// @brief 指定した基底プレハブから派生した全バリアントIDを取得する
    /// @param basePrefabId 基底プレハブID
    /// @return バリアントIDの配列
    gx::Vector<uint64_t> GetVariantsOf(uint64_t basePrefabId) const;

    // --- 上書き管理 ---

    /// @brief バリアントにプロパティ上書きを追加する
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @param propertyName 対象プロパティ名
    /// @param value 上書き値
    /// @return 成功した場合 true
    bool AddPropertyOverride(uint64_t variantId, const gx::String& componentName,
                             const gx::String& propertyName, const gx::String& value);

    /// @brief バリアントからプロパティ上書きを削除する
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @param propertyName 対象プロパティ名
    /// @return 見つかって削除された場合 true
    bool RemovePropertyOverride(uint64_t variantId, const gx::String& componentName,
                                const gx::String& propertyName);

    /// @brief コンポーネント単位の上書きを追加する（コンポーネントの追加/削除）
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @param isAdded true の場合、コンポーネント追加の上書き
    /// @return 成功した場合 true
    bool AddComponentOverride(uint64_t variantId, const gx::String& componentName, bool isAdded);

    /// @brief コンポーネント上書きを削除する
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @return 見つかって削除された場合 true
    bool RemoveComponentOverride(uint64_t variantId, const gx::String& componentName);

    /// @brief バリアントの全上書き情報を取得する
    /// @param variantId バリアントID
    /// @return コンポーネント上書きの配列
    gx::Vector<ComponentOverride> GetOverrides(uint64_t variantId) const;

    /// @brief バリアントの上書き数を取得する
    /// @param variantId バリアントID
    /// @return 上書き数（バリアント未発見時は 0）
    size_t GetOverrideCount(uint64_t variantId) const;

    /// @brief 特定のプロパティが上書きされているか判定する
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @param propertyName 対象プロパティ名
    /// @return 上書きされている場合 true
    bool IsOverridden(uint64_t variantId, const gx::String& componentName,
                      const gx::String& propertyName) const;

    /// @brief 特定のプロパティ上書きを元に戻す（削除する）
    /// @param variantId バリアントID
    /// @param componentName 対象コンポーネント名
    /// @param propertyName 対象プロパティ名
    /// @return 見つかって削除された場合 true
    bool RevertOverride(uint64_t variantId, const gx::String& componentName,
                        const gx::String& propertyName);

    /// @brief バリアントの全上書きを元に戻す
    /// @param variantId バリアントID
    void RevertAllOverrides(uint64_t variantId);

    /// @brief 上書きを基底プレハブデータに適用し、変更済みデータを返す
    /// @param variantId バリアントID
    /// @param targetData 上書きを適用する基底データ
    /// @return 上書き適用後のデータ
    gx::Vector<uint8_t> ApplyOverrides(uint64_t variantId, const gx::Vector<uint8_t>& targetData) const;

    // --- ネストバリアント ---

    /// @brief ネストバリアント（バリアントのバリアント）を作成する
    /// @param parentVariantId 親バリアントID
    /// @param name ネストバリアント名
    /// @return 新しいバリアントID（失敗時は 0）
    uint64_t CreateNestedVariant(uint64_t parentVariantId, const gx::String& name);

    /// @brief ルートプレハブから指定バリアントまでの継承チェーンを取得する
    /// @param variantId バリアントID
    /// @return ルートからこのバリアントまでのIDの配列
    gx::Vector<uint64_t> GetInheritanceChain(uint64_t variantId) const;

    // --- シリアライズ ---

    /// @brief 全プレハブとバリアントをファイルに保存する
    /// @param filePath 出力ファイルパス
    /// @return 成功した場合 true
    bool Save(const gx::String& filePath) const;

    /// @brief ファイルからプレハブとバリアントを読み込む
    /// @param filePath 入力ファイルパス
    /// @return 成功した場合 true
    bool Load(const gx::String& filePath);

    /// @brief 全プレハブとバリアントをクリアする
    void Clear();

private:
    /// @brief バリアント内のコンポーネント上書きエントリを検索または作成する
    /// @param variant 対象バリアントデータ
    /// @param componentName コンポーネント名
    /// @return コンポーネント上書きへのポインタ
    ComponentOverride* FindOrCreateComponentOverride(PrefabVariantData& variant,
                                                     const gx::String& componentName);

    /// @brief バリアント内のコンポーネント上書きエントリを検索する（読み取り専用）
    /// @param variant 対象バリアントデータ
    /// @param componentName コンポーネント名
    /// @return コンポーネント上書きへのポインタ（見つからない場合 nullptr）
    const ComponentOverride* FindComponentOverride(const PrefabVariantData& variant,
                                                    const gx::String& componentName) const;

    gx::HashMap<uint64_t, PrefabData> m_prefabs;           ///< ID→プレハブデータのマップ
    gx::HashMap<uint64_t, PrefabVariantData> m_variants;   ///< ID→バリアントデータのマップ
    uint64_t m_nextPrefabId = 1;                                  ///< 次に割り当てるプレハブID
    uint64_t m_nextVariantId = 0x100000;                          ///< 次に割り当てるバリアントID（プレハブと衝突しないよう別範囲）
};

} // namespace gx
/// @}
