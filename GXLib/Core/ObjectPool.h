#pragma once
/// @file ObjectPool.h
/// @brief オブジェクトプール（事前確保 + フリーリスト再利用）
///
/// 頻繁な new/delete を避け、弾やパーティクル等の大量オブジェクトを
/// プール管理する。Get() で取得、Release() で返却。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief テンプレートオブジェクトプール
/// @tparam T プールするオブジェクト型（デフォルトコンストラクタ必須）
template<typename T>
class ObjectPool
{
public:
    /// @brief コンストラクタ
    /// @param initialCapacity 初期確保数
    /// @param autoExpand 容量不足時に自動拡張するか
    explicit ObjectPool(size_t initialCapacity = 64, bool autoExpand = true)
        : m_autoExpand(autoExpand)
    {
        Grow(initialCapacity);
    }

    /// @brief プールからオブジェクトを取得する
    /// @return オブジェクトへのポインタ。容量不足かつ自動拡張OFFの場合nullptr
    T* Get()
    {
        if (m_freeList.empty())
        {
            if (!m_autoExpand) return nullptr;
            Grow(m_storage.size());
        }
        size_t idx = m_freeList.back();
        m_freeList.pop_back();
        m_active[idx] = true;
        m_activeCount++;
        m_storage[idx] = T{};
        return &m_storage[idx];
    }

    /// @brief オブジェクトをプールに返却する
    /// @param obj Get()で取得したポインタ
    void Release(T* obj)
    {
        if (!obj) return;
        ptrdiff_t idx = obj - m_storage.data();
        if (idx < 0 || static_cast<size_t>(idx) >= m_storage.size()) return;
        if (!m_active[static_cast<size_t>(idx)]) return;
        m_active[static_cast<size_t>(idx)] = false;
        m_activeCount--;
        m_freeList.push_back(static_cast<size_t>(idx));
    }

    /// @brief 全オブジェクトをプールに返却する
    void ReleaseAll()
    {
        m_freeList.clear();
        m_activeCount = 0;
        for (size_t i = 0; i < m_storage.size(); ++i)
        {
            m_active[i] = false;
            m_freeList.push_back(i);
        }
    }

    /// @brief アクティブな全オブジェクトに対して関数を実行する
    /// @param func 各オブジェクトに適用する関数
    template<typename Func>
    void ForEachActive(Func func)
    {
        for (size_t i = 0; i < m_storage.size(); ++i)
        {
            if (m_active[i])
                func(m_storage[i]);
        }
    }

    /// @brief 条件を満たすオブジェクトを返却する
    /// @param predicate trueを返したオブジェクトをRelease
    template<typename Pred>
    void ReleaseIf(Pred predicate)
    {
        for (size_t i = 0; i < m_storage.size(); ++i)
        {
            if (m_active[i] && predicate(m_storage[i]))
            {
                m_active[i] = false;
                m_activeCount--;
                m_freeList.push_back(i);
            }
        }
    }

    /// @brief アクティブなオブジェクト数を取得する
    /// @return 使用中のオブジェクト数
    size_t GetActiveCount() const { return m_activeCount; }

    /// @brief プールの総容量を取得する
    /// @return 確保済みスロットの総数
    size_t GetCapacity() const { return m_storage.size(); }

    /// @brief 未使用スロット数を取得する
    /// @return フリーリスト内のスロット数
    size_t GetFreeCount() const { return m_freeList.size(); }

private:
    void Grow(size_t count)
    {
        if (count == 0) count = 1;
        size_t oldSize = m_storage.size();
        m_storage.resize(oldSize + count);
        m_active.resize(oldSize + count, false);
        for (size_t i = oldSize; i < oldSize + count; ++i)
            m_freeList.push_back(i);
    }

    std::vector<T>      m_storage;          ///< オブジェクト格納用配列
    std::vector<bool>   m_active;           ///< 各スロットの使用状態
    std::vector<size_t> m_freeList;         ///< 未使用スロットのインデックスリスト
    size_t              m_activeCount = 0;  ///< アクティブなオブジェクト数
    bool                m_autoExpand  = true; ///< 容量不足時の自動拡張フラグ
};

} // namespace gx
/// @}
