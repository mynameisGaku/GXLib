#pragma once
/// @file Coroutine.h
/// @brief C++20 コルーチンベースの軽量タスクシステム
///
/// co_await WaitForSeconds(n) でフレームをまたいだ待機が可能。
/// CoroutineManager で複数コルーチンの一括管理・更新を行う。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <coroutine>

namespace gx
{

/// @brief コルーチンの待機状態
struct WaitForSeconds
{
    float seconds;         ///< 待機する秒数
    float elapsed = 0.0f;  ///< 経過秒数（内部管理用）
};

/// @brief C++20 コルーチンハンドル
struct Coroutine
{
    struct promise_type
    {
        WaitForSeconds* currentWait = nullptr;  ///< 現在の待機状態（nullptr=待機なし）

        /// @brief コルーチンオブジェクトを生成する（コンパイラが呼ぶ）
        /// @return 生成されたCoroutine
        Coroutine get_return_object()
        {
            return Coroutine{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        /// @brief 開始時にサスペンドする（遅延実行）
        std::suspend_always initial_suspend() noexcept { return {}; }
        /// @brief 終了時にサスペンドする（ハンドルの破棄は呼び出し側が行う）
        std::suspend_always final_suspend() noexcept { return {}; }
        /// @brief co_return 時の処理（void型なので何もしない）
        void return_void() {}
        /// @brief 未処理例外のハンドラー（即座に終了）
        void unhandled_exception() { std::terminate(); }

        /// @brief co_await WaitForSeconds を処理するアウェイター
        auto await_transform(WaitForSeconds wait)
        {
            struct Awaiter
            {
                promise_type* promise;
                WaitForSeconds wait;

                bool await_ready() const noexcept { return wait.seconds <= 0.0f; }
                void await_suspend(std::coroutine_handle<>) noexcept
                {
                    wait.elapsed = 0.0f;
                    promise->currentWait = &wait;
                }
                void await_resume() noexcept
                {
                    promise->currentWait = nullptr;
                }

                // WaitForSeconds をAwaiter内部にコピーして保持
                Awaiter(promise_type* p, WaitForSeconds w) : promise(p), wait(w) {}
            };
            return Awaiter{ this, wait };
        }

        /// @brief 単純なフレーム待機 (co_await std::suspend_always{})
        auto await_transform(std::suspend_always) noexcept
        {
            return std::suspend_always{};
        }
    };

    using Handle = std::coroutine_handle<promise_type>;  ///< コルーチンハンドル型エイリアス
    Handle handle;  ///< 内部コルーチンハンドル（nullptr=無効）

    Coroutine() : handle(nullptr) {}
    explicit Coroutine(Handle h) : handle(h) {}

    Coroutine(Coroutine&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    Coroutine& operator=(Coroutine&& other) noexcept
    {
        if (this != &other)
        {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    ~Coroutine()
    {
        if (handle) handle.destroy();
    }

    /// @brief 1ステップ実行する
    /// @return コルーチンが完了した場合 true
    bool Resume()
    {
        if (!handle || handle.done()) return true;
        handle.resume();
        return handle.done();
    }

    /// @brief コルーチンが完了済みかどうか
    /// @return 完了済みまたは無効な場合 true
    bool IsDone() const
    {
        return !handle || handle.done();
    }
};

/// @brief 複数コルーチンの管理・更新
class CoroutineManager
{
public:
    /// @brief コルーチンを開始する
    /// @param coroutine 開始するコルーチン (ムーブで所有権移転)
    void Start(Coroutine coroutine)
    {
        m_coroutines.push_back(std::move(coroutine));
    }

    /// @brief 全コルーチンを1フレーム分更新する
    /// @param deltaTime 経過時間（秒）
    void Update(float deltaTime)
    {
        for (auto it = m_coroutines.begin(); it != m_coroutines.end(); )
        {
            auto& co = *it;
            if (co.IsDone())
            {
                it = m_coroutines.erase(it);
                continue;
            }

            auto& promise = co.handle.promise();
            if (promise.currentWait)
            {
                promise.currentWait->elapsed += deltaTime;
                if (promise.currentWait->elapsed >= promise.currentWait->seconds)
                {
                    // タイマー完了 → resume
                    promise.currentWait = nullptr;
                    co.handle.resume();
                }
            }
            else
            {
                co.handle.resume();
            }

            if (co.IsDone())
                it = m_coroutines.erase(it);
            else
                ++it;
        }
    }

    /// @brief 全コルーチンを停止・破棄する
    void StopAll()
    {
        m_coroutines.clear();
    }

    /// @brief アクティブなコルーチン数を取得する
    /// @return 実行中のコルーチン数
    size_t GetActiveCount() const { return m_coroutines.size(); }

private:
    gx::Vector<Coroutine> m_coroutines;  ///< 管理中のコルーチン配列
};

} // namespace gx
/// @}
