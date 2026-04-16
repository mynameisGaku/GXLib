#pragma once
/// @file ResourceHandle.h
/// @brief 型安全な参照カウント付きリソースハンドル
///
/// TextureManager, AudioManager等の個別管理を統一する基盤。
/// ResourceHandle<T>はリソースIDと参照カウント管理を提供する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief リソースの状態
enum class ResourceState
{
    Unloaded,    ///< 未ロード
    Loading,     ///< 非同期ロード中
    Loaded,      ///< ロード完了
    Failed,      ///< ロード失敗
};

/// @brief リソースハンドル基底（型消去用）
struct ResourceHandleBase
{
    uint32_t id = 0;          ///< リソースID (0=無効)
    uint32_t generation = 0;  ///< 世代番号（再利用検出用）

    /// @brief ハンドルが有効か判定する
    /// @return ID が 0 でなければ true
    bool IsValid() const { return id != 0; }

    /// @brief bool 変換演算子（IsValid() と同等）
    explicit operator bool() const { return IsValid(); }

    /// @brief 等値比較演算子
    /// @param o 比較対象
    /// @return ID と世代番号が両方一致すれば true
    bool operator==(const ResourceHandleBase& o) const { return id == o.id && generation == o.generation; }

    /// @brief 非等値比較演算子
    /// @param o 比較対象
    /// @return 一致しなければ true
    bool operator!=(const ResourceHandleBase& o) const { return !(*this == o); }
};

/// @brief 型安全なリソースハンドル
template<typename T>
struct ResourceHandle : ResourceHandleBase
{
    ResourceHandle() = default;

    /// @brief リソースIDと世代番号を指定してハンドルを生成する
    /// @param resourceId リソースID
    /// @param gen 世代番号（デフォルト: 0）
    explicit ResourceHandle(uint32_t resourceId, uint32_t gen = 0)
    {
        id = resourceId;
        generation = gen;
    }
};

/// @brief リソースエントリ（内部管理用）
template<typename T>
struct ResourceEntry
{
    gx::String name;              ///< リソース名/パス
    gx::String path;              ///< ファイルパス
    ResourceState state = ResourceState::Unloaded; ///< リソースの現在の状態
    uint32_t refCount = 0;         ///< 参照カウント
    uint32_t generation = 0;       ///< 世代番号
    std::unique_ptr<T> resource;   ///< 実リソース
};

/// @brief 汎用リソースキャッシュ
/// @tparam T リソース型
template<typename T>
class ResourceCache
{
public:
    /// @brief 名前でリソースを検索（キャッシュヒット）
    /// @param name リソース名
    /// @return 見つかった場合は有効なハンドル、見つからない場合は無効なハンドル
    ResourceHandle<T> Find(const gx::String& name) const
    {
        auto it = m_nameToIndex.find(name);
        if (it != m_nameToIndex.end()) {
            auto& entry = m_entries[it->second];
            if (entry.state == ResourceState::Loaded) {
                return ResourceHandle<T>(it->second + 1, entry.generation);
            }
        }
        return ResourceHandle<T>();
    }

    /// @brief リソースを登録
    /// @param name リソース名
    /// @param resource リソースの所有権
    /// @return 登録されたリソースのハンドル
    ResourceHandle<T> Register(const gx::String& name, std::unique_ptr<T> resource)
    {
        auto it = m_nameToIndex.find(name);
        if (it != m_nameToIndex.end()) {
            auto& entry = m_entries[it->second];
            entry.resource = std::move(resource);
            entry.state = ResourceState::Loaded;
            entry.refCount++;
            return ResourceHandle<T>(it->second + 1, entry.generation);
        }
        // 新スロット（まずフリーリストを確認）
        uint32_t index;
        if (!m_freeList.empty()) {
            index = m_freeList.back();
            m_freeList.pop_back();
            m_entries[index].name = name;
            m_entries[index].resource = std::move(resource);
            m_entries[index].state = ResourceState::Loaded;
            m_entries[index].refCount = 1;
            m_entries[index].generation++;
        } else {
            index = static_cast<uint32_t>(m_entries.size());
            ResourceEntry<T> entry;
            entry.name = name;
            entry.resource = std::move(resource);
            entry.state = ResourceState::Loaded;
            entry.refCount = 1;
            m_entries.push_back(std::move(entry));
        }
        m_nameToIndex[name] = index;
        return ResourceHandle<T>(index + 1, m_entries[index].generation);
    }

    /// @brief 参照カウント増加
    /// @param handle 対象のリソースハンドル
    void AddRef(ResourceHandle<T> handle)
    {
        if (auto* e = GetEntry(handle)) e->refCount++;
    }

    /// @brief 参照カウント減少（0になったら解放予約）
    /// @param handle 対象のリソースハンドル
    void Release(ResourceHandle<T> handle)
    {
        if (auto* e = GetEntry(handle)) {
            if (e->refCount > 0) e->refCount--;
        }
    }

    /// @brief 参照カウントが0のリソースを実際に解放
    /// @return 解放されたリソースの数
    uint32_t CollectGarbage()
    {
        uint32_t freed = 0;
        for (uint32_t i = 0; i < m_entries.size(); i++) {
            auto& e = m_entries[i];
            if (e.state == ResourceState::Loaded && e.refCount == 0) {
                m_nameToIndex.erase(e.name);
                e.resource.reset();
                e.state = ResourceState::Unloaded;
                e.name.clear();
                m_freeList.push_back(i);
                freed++;
            }
        }
        return freed;
    }

    /// @brief リソースを取得
    /// @param handle リソースハンドル
    /// @return リソースポインタ（無効なハンドルの場合は nullptr）
    T* Get(ResourceHandle<T> handle) const
    {
        if (auto* e = GetEntry(handle)) return e->resource.get();
        return nullptr;
    }

    /// @brief 名前でリソースを取得
    /// @param name リソース名
    /// @return リソースポインタ（見つからない場合は nullptr）
    T* GetByName(const gx::String& name) const
    {
        auto it = m_nameToIndex.find(name);
        if (it != m_nameToIndex.end()) {
            auto& entry = m_entries[it->second];
            if (entry.state == ResourceState::Loaded) return entry.resource.get();
        }
        return nullptr;
    }

    /// @brief 登録済みリソース数
    /// @return ロード済みリソースの数
    uint32_t GetCount() const
    {
        uint32_t count = 0;
        for (auto& e : m_entries)
            if (e.state == ResourceState::Loaded) count++;
        return count;
    }

    /// @brief 総エントリ数(空スロット含む)
    /// @return スロットの総数
    uint32_t GetCapacity() const { return static_cast<uint32_t>(m_entries.size()); }

    /// @brief 既存ハンドルのリソースを差し替える (ホットリロード用)
    /// @param handle 差し替え対象のハンドル
    /// @param resource 新しいリソースの所有権
    /// @return 差し替え成功で true
    bool Replace(ResourceHandle<T> handle, std::unique_ptr<T> resource)
    {
        auto* e = GetEntry(handle);
        if (!e || e->state != ResourceState::Loaded)
            return false;
        e->resource = std::move(resource);
        return true;
    }

    /// @brief 全リソースを解放
    void Clear()
    {
        m_entries.clear();
        m_nameToIndex.clear();
        m_freeList.clear();
    }

    /// @brief リソースの状態を取得
    /// @param handle リソースハンドル
    /// @return リソースの現在の状態
    ResourceState GetState(ResourceHandle<T> handle) const
    {
        if (auto* e = GetEntry(handle)) return e->state;
        return ResourceState::Unloaded;
    }

    /// @brief 参照カウントを取得
    /// @param handle リソースハンドル
    /// @return 現在の参照カウント
    uint32_t GetRefCount(ResourceHandle<T> handle) const
    {
        if (auto* e = GetEntry(handle)) return e->refCount;
        return 0;
    }

private:
    const ResourceEntry<T>* GetEntry(ResourceHandle<T> handle) const
    {
        if (handle.id == 0) return nullptr;
        uint32_t idx = handle.id - 1;
        if (idx >= m_entries.size()) return nullptr;
        auto& e = m_entries[idx];
        if (e.generation != handle.generation) return nullptr;
        return &e;
    }
    ResourceEntry<T>* GetEntry(ResourceHandle<T> handle)
    {
        return const_cast<ResourceEntry<T>*>(
            static_cast<const ResourceCache*>(this)->GetEntry(handle));
    }

    mutable gx::Vector<ResourceEntry<T>> m_entries;              ///< リソースエントリ配列
    gx::HashMap<gx::String, uint32_t> m_nameToIndex;     ///< 名前→インデックスのマップ
    gx::Vector<uint32_t> m_freeList;                            ///< 再利用可能なスロットのリスト
};

} // namespace gx
/// @}
