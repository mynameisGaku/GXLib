#pragma once
/// @file EventBus.h
/// @brief 型安全なイベントバス（Publish-Subscribe パターン）
///
/// ゲーム内イベントの発行と購読を管理する。型ベースのディスパッチにより、
/// 任意の構造体をイベントとして使用できる。
/// 同期発行 (Fire) と遅延発行 (Queue + DispatchQueued) の2モードを提供。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <typeindex>
#include <any>

namespace gx
{

/// @brief イベント購読のハンドル（Unsubscribe用）
struct EventHandle
{
    uint64_t id = 0;  ///< 購読ID（0は無効）
    explicit operator bool() const { return id != 0; }
};

/// @brief 型安全なイベントバス（シングルトン）
///
/// Fire<T>(event) で同期発行、Queue<T>(event) で遅延発行し
/// DispatchQueued() でまとめて配信する。
class EventBus
{
public:
    /// @brief シングルトンインスタンスを取得する
    static EventBus& Instance()
    {
        static EventBus instance;
        return instance;
    }

    /// @brief イベントを購読する
    /// @tparam T イベント型
    /// @param callback イベント受信時に呼ばれるコールバック
    /// @return 購読ハンドル（Unsubscribe用）
    template<typename T>
    EventHandle Subscribe(std::function<void(const T&)> callback)
    {
        auto key = std::type_index(typeid(T));
        uint64_t id = ++m_nextId;
        auto wrapper = [cb = std::move(callback)](const void* ptr) {
            cb(*static_cast<const T*>(ptr));
        };
        m_handlers[key].push_back({ id, std::move(wrapper) });
        return EventHandle{ id };
    }

    /// @brief 購読を解除する
    /// @param handle Subscribe() で取得したハンドル
    void Unsubscribe(EventHandle handle)
    {
        for (auto& [key, handlers] : m_handlers)
        {
            for (auto it = handlers.begin(); it != handlers.end(); ++it)
            {
                if (it->id == handle.id)
                {
                    handlers.erase(it);
                    return;
                }
            }
        }
    }

    /// @brief イベントを同期発行する（即座に全ハンドラを呼ぶ）
    /// @tparam T イベント型
    /// @param event 発行するイベントデータ
    template<typename T>
    void Fire(const T& event)
    {
        auto key = std::type_index(typeid(T));
        auto it = m_handlers.find(key);
        if (it == m_handlers.end()) return;

        // コピーして反復（再入安全）
        auto copy = it->second;
        for (auto& handler : copy)
            handler.callback(&event);
    }

    /// @brief イベントを遅延キューに追加する
    /// @tparam T イベント型
    /// @param event キューに追加するイベントデータ
    template<typename T>
    void Queue(const T& event)
    {
        auto key = std::type_index(typeid(T));
        auto ptr = std::make_shared<T>(event);
        m_queue.push_back({ key, [ptr](const void**) { return static_cast<const void*>(ptr.get()); }, ptr });
    }

    /// @brief キューに溜まったイベントを全て配信する
    void DispatchQueued()
    {
        auto queue = std::move(m_queue);
        m_queue.clear();
        for (auto& entry : queue)
        {
            auto it = m_handlers.find(entry.type);
            if (it == m_handlers.end()) continue;
            auto copy = it->second;
            const void* data = entry.data.get();
            for (auto& handler : copy)
                handler.callback(data);
        }
    }

    /// @brief 全てのハンドラとキューをクリアする
    void Clear()
    {
        m_handlers.clear();
        m_queue.clear();
    }

private:
    EventBus() = default;

    /// @brief 内部ハンドラ情報
    struct Handler
    {
        uint64_t id;                                 ///< 購読ID
        std::function<void(const void*)> callback;   ///< 型消去されたコールバック
    };

    /// @brief 遅延キュー内のイベント情報
    struct QueuedEvent
    {
        std::type_index type = std::type_index(typeid(void));  ///< イベントの型インデックス
        std::function<const void*(const void**)> resolver;     ///< データ解決関数
        std::shared_ptr<void> data;                            ///< イベントデータの所有権
    };

    std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;  ///< 型→ハンドラリストのマップ
    std::vector<QueuedEvent> m_queue;    ///< 遅延発行キュー
    uint64_t m_nextId = 0;               ///< 次に割り当てる購読ID
};

} // namespace gx
/// @}
