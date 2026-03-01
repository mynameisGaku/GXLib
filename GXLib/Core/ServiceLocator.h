#pragma once
/// @file ServiceLocator.h
/// @brief サービスロケーターパターン（DI基盤）
///
/// エンジン各サブシステムの登録と取得を一元管理する。
/// 具象型への直接依存を排除し、テスト時にモック実装を注入可能にする。
///
/// 使用例:
/// @code
///   // 登録（初期化時）
///   ServiceLocator::Instance().Register<IAudioDevice>(audioDevicePtr);
///
///   // 取得（利用時）
///   auto& audio = ServiceLocator::Instance().Get<IAudioDevice>();
/// @endcode

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <format>

namespace gx {
/// @addtogroup grp_core
/// @{

/// @brief サービスロケーター（シングルトン）
class ServiceLocator
{
public:
    /// @brief シングルトンインスタンスを取得する
    static ServiceLocator& Instance()
    {
        static ServiceLocator instance;
        return instance;
    }

    /// @brief サービスを登録する
    /// @tparam T インターフェース型（IAudioDevice等）
    /// @param service サービスの共有ポインタ
    template<typename T>
    void Register(std::shared_ptr<T> service)
    {
        m_services[std::type_index(typeid(T))] = std::move(service);
    }

    /// @brief サービスを取得する
    /// @tparam T インターフェース型
    /// @return サービスへの参照
    /// @throws std::runtime_error サービスが未登録の場合
    template<typename T>
    T& Get()
    {
        auto it = m_services.find(std::type_index(typeid(T)));
        if (it == m_services.end())
        {
            throw std::runtime_error(
                std::format("ServiceLocator: Service not registered: {}", typeid(T).name()));
        }
        return *static_cast<T*>(it->second.get());
    }

    /// @brief サービスが登録済みか確認する
    /// @tparam T インターフェース型
    template<typename T>
    bool Has() const
    {
        return m_services.contains(std::type_index(typeid(T)));
    }

    /// @brief 全サービスの登録を解除する（シャットダウン用）
    void Clear()
    {
        m_services.clear();
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;
};

/// @}
} // namespace gx
