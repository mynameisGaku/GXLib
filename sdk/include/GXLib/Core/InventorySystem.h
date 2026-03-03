#pragma once
/// @file InventorySystem.h
/// @brief アイテムインベントリ管理システム
///
/// ゲーム内アイテムの定義・所持・スタック・メタデータを管理する。
/// スロット数上限・重量計算・カテゴリ検索に対応。
/// OnChanged コールバックで変更通知を受け取れる。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>
#include <unordered_map>

namespace gx
{

/// @brief アイテム定義（マスターデータ）
struct ItemDef
{
    gx::String itemId;                                    ///< 一意なアイテムID
    gx::String displayName;                               ///< 表示名
    gx::String description;                               ///< 説明文
    int         maxStack = 99;                             ///< 最大スタック数
    float       weight   = 0.0f;                           ///< 1個あたりの重量
    gx::String category;                                  ///< カテゴリ（"weapon", "potion" 等）
    gx::HashMap<gx::String, gx::String> properties; ///< 追加プロパティ
};

/// @brief インベントリ内の1スロット
struct InventorySlot
{
    gx::String itemId;                                    ///< アイテムID
    int         quantity = 0;                              ///< 所持数
    gx::HashMap<gx::String, gx::String> metadata; ///< スロット固有メタデータ
};

/// @brief インベントリ変更イベント
struct InventoryChangedEvent
{
    gx::String itemId;       ///< 対象アイテムID
    int         oldQuantity;  ///< 変更前の数量
    int         newQuantity;  ///< 変更後の数量
    enum Action {
        Added,    ///< アイテム追加
        Removed,  ///< アイテム削除
        Modified  ///< 数量変更
    } action;     ///< 変更種別
};

/// @brief アイテムインベントリ管理クラス
class InventorySystem
{
public:
    /// @brief アイテム定義を登録する
    /// @param def 登録するアイテム定義
    void RegisterItemDef(const ItemDef& def);

    /// @brief アイテム定義を取得する（未登録なら nullptr）
    /// @param itemId アイテムID
    /// @return アイテム定義へのポインタ（未登録時は nullptr）
    const ItemDef* GetItemDef(const gx::String& itemId) const;

    /// @brief 登録済みアイテム定義数を返す
    /// @return アイテム定義数
    size_t GetItemDefCount() const;

    /// @brief アイテムを追加する（スタック上限・スロット上限を考慮）
    /// @param itemId アイテムID
    /// @param quantity 追加する数量
    /// @return 追加に成功した場合 true
    bool AddItem(const gx::String& itemId, int quantity = 1);

    /// @brief アイテムを削除する
    /// @param itemId アイテムID
    /// @param quantity 削除する数量
    /// @return 削除に成功した場合 true
    bool RemoveItem(const gx::String& itemId, int quantity = 1);

    /// @brief 指定数量以上のアイテムを所持しているか
    /// @param itemId アイテムID
    /// @param minQuantity 必要な最小数量
    /// @return 所持数が minQuantity 以上なら true
    bool HasItem(const gx::String& itemId, int minQuantity = 1) const;

    /// @brief アイテムの所持数を返す（未所持なら 0）
    /// @param itemId アイテムID
    /// @return 所持数
    int GetItemCount(const gx::String& itemId) const;

    /// @brief 指定アイテムのスロットを取得する（未所持なら nullptr）
    /// @param itemId アイテムID
    /// @return スロットへのポインタ（未所持時は nullptr）
    const InventorySlot* GetSlot(const gx::String& itemId) const;

    /// @brief アイテムのメタデータを設定する
    /// @param itemId アイテムID
    /// @param key メタデータキー
    /// @param value メタデータ値
    void SetItemMetadata(const gx::String& itemId, const gx::String& key, const gx::String& value);

    /// @brief アイテムのメタデータを取得する（未設定なら空文字列）
    /// @param itemId アイテムID
    /// @param key メタデータキー
    /// @return メタデータ値（未設定時は空文字列）
    gx::String GetItemMetadata(const gx::String& itemId, const gx::String& key) const;

    /// @brief 全スロットを取得する
    /// @return 全スロットの配列
    gx::Vector<InventorySlot> GetAllItems() const;

    /// @brief 指定カテゴリのスロットを取得する
    /// @param category カテゴリ名
    /// @return 一致するスロットの配列
    gx::Vector<InventorySlot> GetItemsByCategory(const gx::String& category) const;

    /// @brief 使用中スロット数を返す
    /// @return 使用中のスロット数
    size_t GetSlotCount() const;

    /// @brief 最大スロット数を返す
    /// @return 最大スロット数
    size_t GetMaxSlots() const;

    /// @brief 最大スロット数を設定する
    /// @param maxSlots 設定する最大スロット数
    void SetMaxSlots(size_t maxSlots);

    /// @brief 全アイテムの合計重量を返す
    /// @return 合計重量
    float GetTotalWeight() const;

    /// @brief インベントリを全消去する
    void Clear();

    std::function<void(const InventoryChangedEvent&)> OnChanged; ///< 変更通知コールバック

private:
    /// @brief スロットを検索する（可変版）
    InventorySlot* FindSlot(const gx::String& itemId);

    /// @brief スロットを検索する（不変版）
    const InventorySlot* FindSlot(const gx::String& itemId) const;

    /// @brief 変更イベントを発火する
    void NotifyChanged(const gx::String& itemId, int oldQty, int newQty, InventoryChangedEvent::Action action);

    gx::HashMap<gx::String, ItemDef> m_itemDefs; ///< アイテム定義テーブル
    gx::Vector<InventorySlot> m_slots;                   ///< 所持スロット配列
    size_t m_maxSlots = 100;                              ///< 最大スロット数
};

} // namespace gx
/// @}
