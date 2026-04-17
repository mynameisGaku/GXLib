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
#include <mutex>
#include <thread>
#include <cassert>

namespace gx
{

/// @brief ハンドラのカテゴリ (リプレイ抑制用、ADR-0016 §3)
enum class HandlerCategory
{
    Idempotent,  ///< 決定的な状態変更のみ — リプレイ中も実行される
    SideEffect   ///< 外部に影響する処理 (音声/VFX/ネット送信) — リプレイ中は抑制
};

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

    /// @brief イベントを購読する (カテゴリ指定、ADR-0016 §3)
    /// @tparam T イベント型
    /// @param callback イベント受信時に呼ばれるコールバック
    /// @param category ハンドラカテゴリ (リプレイ抑制に影響)
    /// @return 購読ハンドル（Unsubscribe用）
    template<typename T>
    EventHandle Subscribe(std::function<void(const T&)> callback, HandlerCategory category)
    {
        auto key = std::type_index(typeid(T));
        uint64_t id = ++m_nextId;
        auto wrapper = [cb = std::move(callback)](const void* ptr) {
            cb(*static_cast<const T*>(ptr));
        };
        m_handlers[key].push_back({ id, std::move(wrapper), category });
        return EventHandle{ id };
    }

    /// @brief イベントを購読する (デフォルト: SideEffect)
    /// @tparam T イベント型
    /// @param callback イベント受信時に呼ばれるコールバック
    /// @return 購読ハンドル（Unsubscribe用）
    template<typename T>
    EventHandle Subscribe(std::function<void(const T&)> callback)
    {
        return Subscribe<T>(std::move(callback), HandlerCategory::SideEffect);
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

    /// @brief リプレイモードを設定する (ADR-0016 §4, ADR-0013 §13)
    /// ロールバック再シミュレーション中に true にすると SideEffect ハンドラが抑制される。
    void SetReplayMode(bool on) { m_replayMode = on; }

    /// @brief 現在リプレイモードか取得する
    bool IsReplayMode() const { return m_replayMode; }

    /// @brief イベントを同期発行する（即座に全ハンドラを呼ぶ）
    /// リプレイモード中は SideEffect カテゴリのハンドラをスキップする。
    /// @tparam T イベント型
    /// @param event 発行するイベントデータ
    template<typename T>
    void Fire(const T& event)
    {
        assert(std::this_thread::get_id() == m_mainThreadId
               && "EventBus::Fire must be called from the main thread (eventbus_fire_from_worker_thread)");
        auto key = std::type_index(typeid(T));
        auto it = m_handlers.find(key);
        if (it == m_handlers.end()) return;

        auto copy = it->second;
        for (auto& handler : copy)
        {
            if (m_replayMode && handler.category == HandlerCategory::SideEffect)
                continue;
            handler.callback(&event);
        }
    }

    /// @brief イベントを遅延キューに追加する
    /// リプレイモード中はキューに追加しない (no-op)。
    /// @tparam T イベント型
    /// @param event キューに追加するイベントデータ
    template<typename T>
    void Queue(const T& event)
    {
        if (m_replayMode) return;
        auto key = std::type_index(typeid(T));
        auto ptr = std::make_shared<T>(event);
        m_queue.push_back({ key, [ptr](const void**) { return static_cast<const void*>(ptr.get()); }, ptr });
    }

    /// @brief ワーカースレッドからイベントを遅延キューに追加する (ADR-0016 §5)
    /// mutex で保護されたキューに push する。メインスレッドの DispatchQueued() でドレインされる。
    /// @tparam T イベント型
    /// @param event キューに追加するイベントデータ
    template<typename T>
    void QueueFromWorker(const T& event)
    {
        auto key = std::type_index(typeid(T));
        auto ptr = std::make_shared<T>(event);
        std::lock_guard<std::mutex> lock(m_workerMutex);
        m_workerQueue.push_back({ key, [ptr](const void**) { return static_cast<const void*>(ptr.get()); }, ptr });
    }

    /// @brief キューに溜まったイベントを全て配信する
    /// ワーカーキューを先にドレインし、次にメインスレッドキューを処理する。
    /// リプレイモード中は SideEffect カテゴリのハンドラをスキップする。
    void DispatchQueued()
    {
        // ワーカーキューをメインキューの先頭にドレイン
        {
            std::lock_guard<std::mutex> lock(m_workerMutex);
            if (!m_workerQueue.empty())
            {
                m_queue.insert(m_queue.begin(), m_workerQueue.begin(), m_workerQueue.end());
                m_workerQueue.clear();
            }
        }
        auto queue = std::move(m_queue);
        m_queue.clear();
        for (auto& entry : queue)
        {
            auto it = m_handlers.find(entry.type);
            if (it == m_handlers.end()) continue;
            auto copy = it->second;
            const void* data = entry.data.get();
            for (auto& handler : copy)
            {
                if (m_replayMode && handler.category == HandlerCategory::SideEffect)
                    continue;
                handler.callback(data);
            }
        }
    }

    /// @brief 全てのハンドラとキューをクリアする
    void Clear()
    {
        m_handlers.clear();
        m_queue.clear();
        std::lock_guard<std::mutex> lock(m_workerMutex);
        m_workerQueue.clear();
    }

private:
    EventBus() : m_mainThreadId(std::this_thread::get_id()) {}

    /// @brief 内部ハンドラ情報
    struct Handler
    {
        uint64_t id;                                          ///< 購読ID
        std::function<void(const void*)> callback;            ///< 型消去されたコールバック
        HandlerCategory category = HandlerCategory::SideEffect; ///< カテゴリ (ADR-0016)
    };

    /// @brief 遅延キュー内のイベント情報
    struct QueuedEvent
    {
        std::type_index type = std::type_index(typeid(void));  ///< イベントの型インデックス
        std::function<const void*(const void**)> resolver;     ///< データ解決関数
        std::shared_ptr<void> data;                            ///< イベントデータの所有権
    };

    gx::HashMap<std::type_index, gx::Vector<Handler>> m_handlers;  ///< 型→ハンドラリストのマップ
    gx::Vector<QueuedEvent> m_queue;         ///< メインスレッド遅延発行キュー
    gx::Vector<QueuedEvent> m_workerQueue;  ///< ワーカースレッドキュー (mutex 保護)
    std::mutex m_workerMutex;               ///< ワーカーキュー用ミューテックス
    std::thread::id m_mainThreadId;         ///< メインスレッドID (Fire assertion 用)
    uint64_t m_nextId = 0;                  ///< 次に割り当てる購読ID
    bool m_replayMode = false;              ///< リプレイモード (ADR-0016 §4)
};

} // namespace gx
/// @}
