#pragma once
/// @file PoolAllocator.h
/// @brief 固定サイズオブジェクト用プールアロケータ
///
/// 同一サイズのオブジェクトを大量に生成・破棄する場面（Widget, Sound, Particle等）で、
/// ヒープの断片化を防ぎ、確保・解放を O(1) で行うアロケータ。
///
/// フリーリスト方式: 未使用スロットをリンクドリストで管理し、
/// Allocate() は先頭を取り出し、Free() は先頭に戻す。

#include "pch.h"

namespace GX
{

/// @brief 固定サイズオブジェクト用プールアロケータ
/// @tparam T プールで管理するオブジェクト型
/// @tparam BlockSize 1ブロックあたりのオブジェクト数（デフォルト: 64）
template<typename T, size_t BlockSize = 64>
class PoolAllocator
{
public:
    PoolAllocator() = default;

    ~PoolAllocator()
    {
        for (auto* block : m_blocks)
            ::operator delete(block);
    }

    // コピー禁止
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    /// @brief プールからメモリを確保する（O(1)）
    /// @return 確保されたメモリへのポインタ
    void* Allocate()
    {
        if (!m_freeList)
            AllocateBlock();

        Node* node = m_freeList;
        m_freeList = node->next;
        ++m_activeCount;
        return reinterpret_cast<void*>(node);
    }

    /// @brief メモリをプールに返却する（O(1)）
    /// @param ptr 返却するメモリへのポインタ
    void Free(void* ptr)
    {
        if (!ptr) return;
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = m_freeList;
        m_freeList = node;
        --m_activeCount;
    }

    /// @brief オブジェクトをプールから構築する
    /// @tparam Args コンストラクタ引数の型
    /// @param args コンストラクタに渡す引数
    /// @return 構築されたオブジェクトへのポインタ
    template<typename... Args>
    T* New(Args&&... args)
    {
        void* mem = Allocate();
        return new (mem) T(std::forward<Args>(args)...);
    }

    /// @brief オブジェクトを破棄してプールに返却する
    /// @param obj 破棄するオブジェクトへのポインタ
    void Delete(T* obj)
    {
        if (!obj) return;
        obj->~T();
        Free(obj);
    }

    /// @brief 現在使用中のオブジェクト数
    /// @return アクティブなオブジェクト数
    size_t GetActiveCount() const { return m_activeCount; }

    /// @brief プールの総容量（確保済みスロット数）
    /// @return 総スロット数
    size_t GetCapacity() const { return m_blocks.size() * BlockSize; }

private:
    /// フリーリストノード
    union Node
    {
        Node* next;
        alignas(T) char storage[sizeof(T)];
    };

    static_assert(sizeof(Node) >= sizeof(T), "Node must be at least as large as T");

    /// 新しいブロックを確保してフリーリストに追加
    void AllocateBlock()
    {
        // 1ブロック分のメモリを確保
        void* raw = ::operator new(sizeof(Node) * BlockSize);
        m_blocks.push_back(raw);

        Node* nodes = reinterpret_cast<Node*>(raw);
        for (size_t i = 0; i < BlockSize - 1; ++i)
            nodes[i].next = &nodes[i + 1];
        nodes[BlockSize - 1].next = m_freeList;
        m_freeList = &nodes[0];
    }

    Node*              m_freeList    = nullptr;
    size_t             m_activeCount = 0;
    std::vector<void*> m_blocks;
};

} // namespace GX
