#pragma once
/// @file ComponentStorage.h
/// @brief 型消去された連続コンポーネントストレージ（SoA列）
///
/// 1種類のコンポーネントを生バイト配列として連続格納する。
/// キャッシュに乗りやすいSoAレイアウトで、O(1)削除のために
/// スワップアンドポップを使う。Archetypeの内部で使われる。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"
#include "ECS/ECSTypes.h"

namespace gx { namespace ecs {

/// @brief 1つの型のコンポーネントを連続配列として格納する（型消去）
///
/// 要素は生バイトとして格納される。連続性を維持しながら
/// O(1)削除のためのスワップアンドポップ除去をサポートする。
class ComponentStorage
{
public:
    /// @param id           コンポーネント型ID
    /// @param componentSize 1コンポーネントのバイトサイズ
    /// @param name          オプションのデバッグ名
    ComponentStorage(ComponentID id, uint32_t componentSize, const gx::String& name = "");
    ~ComponentStorage();

    ComponentStorage(ComponentStorage&& other) noexcept;
    ComponentStorage& operator=(ComponentStorage&& other) noexcept;

    // コピー不可
    ComponentStorage(const ComponentStorage&) = delete;
    ComponentStorage& operator=(const ComponentStorage&) = delete;

    /// @brief デフォルトゼロ初期化された要素を追加し、そのポインタを返す
    /// @return 追加された要素へのポインタ
    void* AddElement();

    /// @brief 指定インデックスの要素を削除する（SwapAndPopのシノニム）
    /// @param index 削除する要素のインデックス
    void RemoveElement(uint32_t index);

    /// @brief 指定インデックスの要素へのポインタを取得する
    /// @param index 要素のインデックス
    /// @return 要素データへのポインタ
    void* GetElement(uint32_t index);
    /// @copydoc GetElement(uint32_t)
    const void* GetElement(uint32_t index) const;

    /// @brief 最後の要素とスワップして指定インデックスの要素を削除する
    /// @param index 削除する要素のインデックス
    void SwapAndPop(uint32_t index);

    /// @brief 格納中の要素数を取得する
    /// @return 要素数
    uint32_t GetCount() const { return m_count; }
    /// @brief 1コンポーネントのバイトサイズを取得する
    /// @return バイト単位のサイズ
    uint32_t GetComponentSize() const { return m_componentSize; }
    /// @brief コンポーネント型IDを取得する
    /// @return コンポーネントID
    ComponentID GetComponentID() const { return m_componentId; }
    /// @brief デバッグ名を取得する
    /// @return 名前文字列への参照
    const gx::String& GetName() const { return m_name; }

    /// @brief 指定容量分のストレージを事前確保する
    /// @param capacity 確保する要素数
    void Reserve(uint32_t capacity);

    /// @brief すべての要素を削除する
    void Clear();

private:
    void EnsureCapacity(uint32_t requiredCount);

    ComponentID m_componentId;        ///< このストレージが格納するコンポーネントの型ID
    uint32_t m_componentSize;         ///< 1コンポーネントのバイトサイズ
    gx::String m_name;               ///< デバッグ用コンポーネント名
    gx::Vector<uint8_t> m_data;      ///< 連続バイト配列（生データ）
    uint32_t m_count = 0;             ///< 現在格納中の要素数
    uint32_t m_capacity = 0;          ///< 確保済みの要素容量
};

}} // namespace gx::ecs
/// @}
